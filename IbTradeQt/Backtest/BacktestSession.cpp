#include "BacktestSession.h"
#include "Backtest/JsonlHistoricalDataSource.h"
#include "Backtest/CsvHistoricalDataSource.h"
#include "Backtest/YahooFinanceDataSource.h"
#include "Pipeline/PipelineFactory.h"
#include "Pipeline/Contracts.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/PipelineRuntimeContext.h"
#include "Backtest/BacktestMarketDataAccessor.h"
#include "Pipeline/UniverseResolver.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"
#include <algorithm>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBacktestSession, "backtest.session")

namespace Backtest {

BacktestSession::BacktestSession(const BacktestConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{}

void BacktestSession::cancel()
{
    m_cancelled = true;
}

void BacktestSession::run()
{
    m_cancelled  = false;
    m_buildFailed = false;

    buildObjectGraph();
    if (m_buildFailed)
        return;

    if (!m_preloadedBars.isEmpty()) {
        // Bars were pre-fetched by HistoricalDataManager — skip network fetch
        int totalBars = 0;
        for (auto it = m_preloadedBars.begin(); it != m_preloadedBars.end(); ++it) {
            for (const auto& bar : it.value()) {
                m_replayer->addBar(bar, true);
                ++totalBars;
            }
        }
        qCDebug(lcBacktestSession) << "BacktestSession: using" << totalBars
                 << "preloaded bars for" << m_preloadedBars.keys();
    } else {
        loadHistoricalData();
        if (m_loadFailed) return;   // failed() already emitted inside loadHistoricalData()
        if (m_cancelled) {
            emit failed("Cancelled during data load");
            return;
        }
    }

    driveReplayLoop();

    if (m_cancelled) {
        emit failed("Cancelled during replay");
        return;
    }

    m_result = m_metrics->finalize(m_config.startDate, m_config.endDate);
    m_result.dataQuality = (m_config.dataSourceId == "jsonl")
        ? DataQuality::RealTicks
        : (m_config.resolution == BarResolution::Day1
           ? DataQuality::DailyBars
           : DataQuality::SynthesizedOHLC);

    // Load benchmark data if configured
    if (!m_config.benchmarkSymbol.isEmpty()) {
        loadBenchmarkData();
        m_result.alphaVsBenchmark =
            m_result.annualizedReturn - m_result.benchmark.annualizedReturn;
    }

    emit finished(m_result);
}

void BacktestSession::buildObjectGraph()
{
    // Create data source based on config
    if (m_config.dataSourceId == "jsonl") {
        m_dataSource = std::make_unique<JsonlHistoricalDataSource>(m_config.dataPath);
    } else if (m_config.dataSourceId == "csv") {
        m_dataSource = std::make_unique<CsvHistoricalDataSource>(m_config.dataPath);
    } else if (m_config.dataSourceId == "yahoo") {
        auto yahoo = std::make_unique<YahooFinanceDataSource>();
        if (m_yahooNetworkManager)
            yahoo->setNetworkManager(m_yahooNetworkManager);
        m_dataSource = std::move(yahoo);
    } else {
        qCWarning(lcBacktestSession) << "BacktestSession: unknown dataSourceId" << m_config.dataSourceId
                   << "— defaulting to JSONL";
        m_dataSource = std::make_unique<JsonlHistoricalDataSource>(m_config.dataPath);
    }

    m_replayer   = std::make_unique<MarketDataReplayer>();
    m_priceStore = std::make_unique<MarketPriceStore>();
    m_clock      = std::make_unique<SimulatedClock>();

    m_execAdapter = std::make_unique<SimulatedExecutionAdapter>(
        m_config.fillModel,
        m_config.slippageBps,
        m_config.fillTiming,
        m_priceStore.get(),
        m_clock.get());

    m_ledger = std::make_unique<SimulatedLedger>(
        m_config.initialCapital,
        m_priceStore.get());

    m_metrics = std::make_unique<BacktestMetricsCollector>(m_config.initialCapital);

    // Load pipeline config — inline takes precedence over file path
    QJsonObject pipelineConfig;
    if (!m_inlinePipelineConfig.isEmpty()) {
        pipelineConfig = m_inlinePipelineConfig;
    } else if (!m_config.strategyConfigPath.isEmpty()) {
        QFile f(m_config.strategyConfigPath);
        if (f.open(QIODevice::ReadOnly)) {
            pipelineConfig = QJsonDocument::fromJson(f.readAll()).object();
        } else {
            qCWarning(lcBacktestSession) << "BacktestSession: cannot open pipeline config:"
                       << m_config.strategyConfigPath;
        }
    }

    // Build the pipeline via CPipelineStrategyAdapter — same interface as live mode.
    // The adapter constructs StrategyPipelineRunner directly (no supervisor, no thread).
    m_strategyAdapter = std::make_unique<CPipelineStrategyAdapter>();
    m_strategyAdapter->setPipelineConfig(pipelineConfig);

    CPipelineStrategyAdapter::BacktestContext ctx;
    ctx.execPort = m_execAdapter.get();
    ctx.clock    = m_clock.get();
    ctx.ledger   = m_ledger.get();
    m_strategyAdapter->injectBacktestContext(ctx);
    m_strategyAdapter->start();

    // m_pipelineRunner is a non-owning view — adapter owns the runner.
    m_pipelineRunner = m_strategyAdapter->backtestPipelineRunner();
    if (!m_pipelineRunner) {
        m_buildFailed = true;
        emit failed(QStringLiteral("Pipeline configuration is invalid or empty."));
        return;
    }

    // Seed the universe from the pipeline selection config or from
    // the data-source symbols so the pipeline has an explicit universe.
    {
        auto resolved = Pipeline::UniverseResolver::resolve(pipelineConfig);
        QVector<QString> universe;
        if (resolved.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols) {
            universe = resolved.symbols;
        } else {
            for (const auto& sym : m_config.symbols)
                universe.append(sym);
        }
        if (universe.isEmpty()) {
            m_buildFailed = true;
            emit failed("Cannot determine tradeable universe: selection block requires "
                        "explicit symbols but none were resolved.");
            return;
        }
        m_pipelineRunner->setUniverse(universe);
    }

    m_marketDataAccessor = std::make_unique<BacktestMarketDataAccessor>(m_priceStore.get());
    m_historicalReadAdapter.reset();
    if (m_historicalDataManager)
        m_historicalReadAdapter =
            std::make_unique<BacktestHistoricalReadAdapter>(m_historicalDataManager);
    m_noOpSubscriptionPort = std::make_unique<Pipeline::NoOpSubscriptionPort>();
    {
        Pipeline::PipelineRuntimeContext ctx;
        ctx.marketData = m_marketDataAccessor.get();
        if (m_historicalReadAdapter)
            ctx.historical = m_historicalReadAdapter.get();
        ctx.subscription = m_noOpSubscriptionPort.get();
        m_pipelineRunner->setRuntimeContext(ctx);
    }

    // --- Wire signals in priority order (all Qt::DirectConnection, same thread) ---
    // Feed → runner only for strategy; runner dispatches ticks/bars to blocks.

    // Priority 0: price cache updated before any consumer sees the tick
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            m_priceStore.get(), &MarketPriceStore::onTick,
            Qt::DirectConnection);

    // Priority 1: flush pending orders from previous bar close
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            this, [this](const Pipeline::MarketTick& tick) {
                m_clock->setCurrentTime(tick.timestamp);
                m_execAdapter->onNextTickOpen(tick);
            }, Qt::DirectConnection);

    // Priority 2: pipeline ingress (tick + authoritative OHLCV bar)
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            m_pipelineRunner, &Pipeline::StrategyPipelineRunner::ingestTick,
            Qt::DirectConnection);
    connect(m_replayer.get(), &MarketDataReplayer::ohlcvBar,
            m_pipelineRunner, &Pipeline::StrategyPipelineRunner::ingestOhlcvBar,
            Qt::DirectConnection);

    // Priority 3: ledger mark-to-market on bar boundary (after runner slot order for same signal:
    // use QueuedConnection would reorder — keep Direct and connect ledger after runner in same wave)
    connect(m_replayer.get(), &MarketDataReplayer::ohlcvBar,
            m_ledger.get(), [this](const Pipeline::OHLCVBar& bar) {
                m_ledger->onBarClose(bar.symbol, bar.timestamp);
            },
            Qt::DirectConnection);

    // SimulatedExecutionAdapter → SimulatedLedger (fills update ledger state)
    connect(m_execAdapter.get(), &SimulatedExecutionAdapter::filled,
            m_ledger.get(), &SimulatedLedger::onFill,
            Qt::DirectConnection);

    // Priority 5: equity snapshot after ledger is updated
    connect(m_ledger.get(), &SimulatedLedger::snapshot,
            m_metrics.get(), &BacktestMetricsCollector::onSnapshot,
            Qt::DirectConnection);

    connect(m_execAdapter.get(), &SimulatedExecutionAdapter::filled,
            m_metrics.get(), &BacktestMetricsCollector::onFill,
            Qt::DirectConnection);
}

void BacktestSession::loadHistoricalData()
{
    QString loadError;
    bool loadDone = false;

    // Yahoo emits one symbol at a time; replay must be globally time-ordered (matches CSV).
    QVector<IBComm::HistoricalBar> yahooBars;
    const bool bufferYahooBars = (m_config.dataSourceId == QStringLiteral("yahoo"));

    connect(m_dataSource.get(), &IHistoricalDataSource::barLoaded,
            this,
            [this, bufferYahooBars, &yahooBars](const IBComm::HistoricalBar& bar) {
                if (bufferYahooBars)
                    yahooBars.append(bar);
                else
                    m_replayer->addBar(bar, true);
            },
            Qt::DirectConnection);

    connect(m_dataSource.get(), &IHistoricalDataSource::tickLoaded,
            this, [this](const Pipeline::MarketTick& tick) {
                m_replayer->addTick(tick);
            }, Qt::DirectConnection);

    connect(m_dataSource.get(), &IHistoricalDataSource::tickByTickLoaded,
            this, [this](const Pipeline::TickByTickTrade& trade) {
                m_replayer->addTickByTick(trade);
            }, Qt::DirectConnection);

    // Track whether loading finished synchronously (before loop.exec())
    connect(m_dataSource.get(), &IHistoricalDataSource::loadFinished,
            this, [&]() { loadDone = true; }, Qt::DirectConnection);
    connect(m_dataSource.get(), &IHistoricalDataSource::loadFailed,
            this, [&](const QString& reason) {
                loadError = reason;
                loadDone = true;
            }, Qt::DirectConnection);

    qCDebug(lcBacktestSession) << "BacktestSession: requesting bars for" << m_config.symbols
             << "from" << m_config.dataPath;

    m_dataSource->requestBars(
        m_config.symbols,
        m_config.startDate,
        m_config.endDate,
        m_config.resolution);

    qCDebug(lcBacktestSession) << "BacktestSession: loadDone=" << loadDone
             << "replayer tick count=" << m_replayer->tickCount();

    // For synchronous sources, loadDone is already true — skip the event loop.
    // For async sources (IB API, Yahoo Finance), spin the event loop until done.
    if (!loadDone) {
        QEventLoop loop;
        connect(m_dataSource.get(), &IHistoricalDataSource::loadFinished,
                &loop, &QEventLoop::quit, Qt::DirectConnection);
        connect(m_dataSource.get(), &IHistoricalDataSource::loadFailed,
                &loop, &QEventLoop::quit, Qt::DirectConnection);
        loop.exec();
    }

    if (!loadError.isEmpty()) {
        m_cancelled  = true;
        m_loadFailed = true;
        emit failed(loadError);
    } else {
        if (bufferYahooBars && !yahooBars.isEmpty()) {
            std::sort(yahooBars.begin(), yahooBars.end(),
                      [](const IBComm::HistoricalBar& a, const IBComm::HistoricalBar& b) {
                          if (a.timestamp != b.timestamp)
                              return a.timestamp < b.timestamp;
                          return a.symbol < b.symbol;
                      });
            for (const auto& b : yahooBars)
                m_replayer->addBar(b, true);
        }
        if (m_config.dataSourceId == QStringLiteral("yahoo")) {
            // Second YahooFinanceDataSource (benchmark) may share the same injected NAM;
            // disconnect strategy load so benchmark replies are not delivered twice.
            if (auto* yahoo = dynamic_cast<YahooFinanceDataSource*>(m_dataSource.get()))
                yahoo->disconnectFinishedHandler();
        }
    }
}

void BacktestSession::driveReplayLoop()
{
    const int total = m_replayer->tickCount();
    qCDebug(lcBacktestSession) << "BacktestSession::driveReplayLoop: tick count =" << total;
    int processed = 0;

    // Connect progress reporting
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            this, [this, total, &processed](const Pipeline::MarketTick&) mutable {
                ++processed;
                if (total > 0) {
                    emit progressChanged(processed * 100 / total);
                }
            }, Qt::DirectConnection);

    m_replayer->replay();
}

void BacktestSession::loadBenchmarkData()
{
    // If benchmark bars were pre-fetched, use them directly
    if (!m_preloadedBenchmarkBars.isEmpty()) {
        BenchmarkComparison cmp;
        cmp.setInitialCapital(m_config.initialCapital);
        for (const auto& bar : m_preloadedBenchmarkBars) {
            if (bar.symbol == m_config.benchmarkSymbol) {
                cmp.addClose(bar.timestamp, bar.close);
            }
        }
        m_result.benchmark = cmp.compute(m_config.benchmarkSymbol);
        qCDebug(lcBacktestSession) << "BacktestSession: benchmark" << m_config.benchmarkSymbol
                 << "(preloaded) total return:" << m_result.benchmark.totalReturn * 100.0 << "%"
                 << "annualised:" << m_result.benchmark.annualizedReturn * 100.0 << "%";
        return;
    }

    // Build a separate data source of the same type as the strategy source
    // to fetch benchmark bars. For Yahoo, reuse the same source type.
    std::unique_ptr<IHistoricalDataSource> bmSource;
    if (m_config.dataSourceId == "yahoo") {
        auto yahoo = std::make_unique<YahooFinanceDataSource>();
        if (m_yahooNetworkManager)
            yahoo->setNetworkManager(m_yahooNetworkManager);
        bmSource = std::move(yahoo);
    } else if (m_config.dataSourceId == "csv") {
        bmSource = std::make_unique<CsvHistoricalDataSource>(m_config.dataPath);
    } else {
        bmSource = std::make_unique<JsonlHistoricalDataSource>(m_config.dataPath);
    }

    BenchmarkComparison cmp;
    cmp.setInitialCapital(m_config.initialCapital);

    bool done  = false;
    QString err;

    connect(bmSource.get(), &IHistoricalDataSource::barLoaded,
            this, [&](const IBComm::HistoricalBar& bar) {
                if (bar.symbol == m_config.benchmarkSymbol)
                    cmp.addClose(bar.timestamp, bar.close);
            }, Qt::DirectConnection);

    connect(bmSource.get(), &IHistoricalDataSource::loadFinished,
            this, [&]() { done = true; }, Qt::DirectConnection);
    connect(bmSource.get(), &IHistoricalDataSource::loadFailed,
            this, [&](const QString& reason) { err = reason; done = true; }, Qt::DirectConnection);

    qCDebug(lcBacktestSession) << "BacktestSession: loading benchmark" << m_config.benchmarkSymbol;
    bmSource->requestBars(
        QStringList{m_config.benchmarkSymbol},
        m_config.startDate,
        m_config.endDate,
        BarResolution::Day1);

    if (!done) {
        QEventLoop loop;
        connect(bmSource.get(), &IHistoricalDataSource::loadFinished,
                &loop, &QEventLoop::quit, Qt::DirectConnection);
        connect(bmSource.get(), &IHistoricalDataSource::loadFailed,
                &loop, &QEventLoop::quit, Qt::DirectConnection);
        loop.exec();
    }

    if (!err.isEmpty()) {
        qCWarning(lcBacktestSession) << "BacktestSession: benchmark load failed:" << err;
        return;
    }

    m_result.benchmark = cmp.compute(m_config.benchmarkSymbol);
    qCDebug(lcBacktestSession) << "BacktestSession: benchmark" << m_config.benchmarkSymbol
             << "total return:" << m_result.benchmark.totalReturn * 100.0 << "%"
             << "annualised:" << m_result.benchmark.annualizedReturn * 100.0 << "%";
}

} // namespace Backtest
