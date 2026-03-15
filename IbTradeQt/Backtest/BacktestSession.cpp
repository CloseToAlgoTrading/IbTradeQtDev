#include "BacktestSession.h"
#include "Backtest/JsonlHistoricalDataSource.h"
#include "Backtest/CsvHistoricalDataSource.h"
#include "Backtest/YahooFinanceDataSource.h"
#include "Pipeline/PipelineFactory.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

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
    m_cancelled = false;

    buildObjectGraph();

    loadHistoricalData();
    if (m_cancelled) {
        emit failed("Cancelled during data load");
        return;
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
        m_dataSource = std::make_unique<YahooFinanceDataSource>();
    } else {
        qWarning() << "BacktestSession: unknown dataSourceId" << m_config.dataSourceId
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

    // Load pipeline config from file
    QJsonObject pipelineConfig;
    if (!m_config.strategyConfigPath.isEmpty()) {
        QFile f(m_config.strategyConfigPath);
        if (f.open(QIODevice::ReadOnly)) {
            pipelineConfig = QJsonDocument::fromJson(f.readAll()).object();
        } else {
            qWarning() << "BacktestSession: cannot open pipeline config:"
                       << m_config.strategyConfigPath;
        }
    }

    // Build the pipeline graph directly (no supervisor/thread needed for backtest)
    // The strategy runs in the same thread as the replay loop for determinism.
    m_pipelineRunner = std::make_unique<Pipeline::StrategyPipelineRunner>(
        Pipeline::PipelineFactory::buildGraph(pipelineConfig, m_execAdapter.get()),
        m_execAdapter.get(),
        m_ledger.get());
    m_pipelineRunner->wireAlphaSignals();

    // Inject SimulatedClock into all alpha blocks
    for (auto* alpha : m_pipelineRunner->graph().alphaBlocks) {
        alpha->setClock(m_clock.get());
    }

    // --- Wire signals in priority order (all Qt::DirectConnection, same thread) ---

    // Priority 0: price cache updated before any consumer sees the tick
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            m_priceStore.get(), &MarketPriceStore::onTick,
            Qt::DirectConnection);

    // Priority 1: flush pending orders from previous barClose
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            this, [this](const IBComm::MarketTick& tick) {
                m_clock->setCurrentTime(tick.timestamp);
                m_execAdapter->onNextTickOpen(tick);
            }, Qt::DirectConnection);

    // Priority 2: strategy alpha blocks receive the tick
    for (auto* alpha : m_pipelineRunner->graph().alphaBlocks) {
        connect(m_replayer.get(), &MarketDataReplayer::tick,
                alpha, &Pipeline::IAlphaBlock::onTick,
                Qt::DirectConnection);
    }

    // Priority 3: strategy barClose — pipeline runner processes bar boundary
    connect(m_replayer.get(), &MarketDataReplayer::barClose,
            m_pipelineRunner.get(), &Pipeline::StrategyPipelineRunner::onBarClose,
            Qt::DirectConnection);

    // Priority 4: ledger mark-to-market after strategy has processed barClose
    connect(m_replayer.get(), &MarketDataReplayer::barClose,
            m_ledger.get(), &SimulatedLedger::onBarClose,
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

    connect(m_dataSource.get(), &IHistoricalDataSource::barLoaded,
            this, [this](const IBComm::HistoricalBar& bar) {
                m_replayer->addBar(bar, true);
            }, Qt::DirectConnection);

    connect(m_dataSource.get(), &IHistoricalDataSource::tickLoaded,
            this, [this](const IBComm::MarketTick& tick) {
                m_replayer->addTick(tick);
            }, Qt::DirectConnection);

    connect(m_dataSource.get(), &IHistoricalDataSource::tickByTickLoaded,
            this, [this](const IBComm::TickByTickTrade& trade) {
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

    qDebug() << "BacktestSession: requesting bars for" << m_config.symbols
             << "from" << m_config.dataPath;

    m_dataSource->requestBars(
        m_config.symbols,
        m_config.startDate,
        m_config.endDate,
        m_config.resolution);

    qDebug() << "BacktestSession: loadDone=" << loadDone
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
        m_cancelled = true;
        emit failed(loadError);
    }
}

void BacktestSession::driveReplayLoop()
{
    const int total = m_replayer->tickCount();
    qDebug() << "BacktestSession::driveReplayLoop: tick count =" << total;
    int processed = 0;

    // Connect progress reporting
    connect(m_replayer.get(), &MarketDataReplayer::tick,
            this, [this, total, &processed](const IBComm::MarketTick&) mutable {
                ++processed;
                if (total > 0) {
                    emit progressChanged(processed * 100 / total);
                }
            }, Qt::DirectConnection);

    m_replayer->replay();
}

void BacktestSession::loadBenchmarkData()
{
    // Build a separate data source of the same type as the strategy source
    // to fetch benchmark bars. For Yahoo, reuse the same source type.
    std::unique_ptr<IHistoricalDataSource> bmSource;
    if (m_config.dataSourceId == "yahoo") {
        bmSource = std::make_unique<YahooFinanceDataSource>();
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

    qDebug() << "BacktestSession: loading benchmark" << m_config.benchmarkSymbol;
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
        qWarning() << "BacktestSession: benchmark load failed:" << err;
        return;
    }

    m_result.benchmark = cmp.compute(m_config.benchmarkSymbol);
    qDebug() << "BacktestSession: benchmark" << m_config.benchmarkSymbol
             << "total return:" << m_result.benchmark.totalReturn * 100.0 << "%"
             << "annualised:" << m_result.benchmark.annualizedReturn * 100.0 << "%";
}

} // namespace Backtest
