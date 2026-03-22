#include "Backtest/BacktestController.h"
#include "Backtest/BacktestRunPersistence.h"
#include "Backtest/HistoricalDataManager.h"
#include "Backtest/BacktestConstants.h"
#include "Pipeline/UniverseResolver.h"
#include "DB/dbquery.h"
#include "DB/dbdatatypes.h"
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QVector>
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <cmath>

Q_LOGGING_CATEGORY(lcBacktestController, "backtest.controller")

namespace Backtest {

BacktestController::BacktestController(const QString& dbFileName,
                                        QNetworkAccessManager* networkManager,
                                        QObject* parent)
    : QObject(parent)
    , m_networkManager(networkManager)
{
    // Open a dedicated connection for the main thread. Using a UUID name
    // avoids clashing with the DBHandler connection.
    m_dbConnectionName = QStringLiteral("bt_ctrl_") +
                         QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_dbConnectionName);
    db.setDatabaseName(dbFileName);
    if (!db.open()) {
        qCWarning(lcBacktestController) << "BacktestController: cannot open DB" << dbFileName;
    }
}

BacktestController::~BacktestController() {
    if (m_workerThread) {
        m_workerThread->disconnect();
        if (m_session) m_session->disconnect();

        if (m_workerThread->isRunning()) {
            if (m_session) m_session->cancel();
            m_workerThread->quit();
            m_workerThread->wait(Backtest::kThreadShutdownTimeoutMs);
        }

        delete m_session;
        m_session = nullptr;

        delete m_workerThread;
        m_workerThread = nullptr;
    }

    // Close and remove the dedicated DB connection
    {
        QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
        if (db.isOpen()) db.close();
    }
    QSqlDatabase::removeDatabase(m_dbConnectionName);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void BacktestController::start(const BacktestRunConfig& config) {
    if (isRunning()) {
        qCWarning(lcBacktestController) << "BacktestController: a run is already in progress — ignoring start()";
        return;
    }

    m_currentConfig = config;
    m_currentRunId  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_dataRefreshedAt.clear();

    const QString persistedStrategyId =
        Persistence::resolvedStrategyIdForPersistence(config, m_currentRunId);

    // Persist initial run record with status "Created"
    DbBacktestRun runRecord;
    runRecord.runId               = m_currentRunId;
    runRecord.strategyId          = persistedStrategyId;
    runRecord.strategyDisplayName = config.strategyDisplayName;
    runRecord.portfolioPath       = config.portfolioPath;
    runRecord.configJson          = QString::fromUtf8(
        QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact));
    runRecord.symbols             = config.symbolsJoined();
    runRecord.startDate           = config.startDate.toUTC().toString(Qt::ISODate);
    runRecord.endDate             = config.endDate.toUTC().toString(Qt::ISODate);
    runRecord.status              = QString(Backtest::Status::Created);
    runRecord.engineVersion       = engineVersion();
    runRecord.dataSourceId        = config.dataSourceId;
    runRecord.createdAt           = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    // Canonical scope fields — caller is responsible for populating these before calling start().
    // Do NOT set scopeRefId = config.strategyId implicitly (they serve different semantics).
    runRecord.strategyDefId       = config.strategyDefId;
    runRecord.scopeType           = config.scopeType.isEmpty()
                                      ? QString(Backtest::Scope::Strategy)
                                      : config.scopeType;
    runRecord.scopeRefId          = config.scopeRefId;
    runRecord.strategyVersion     = config.strategyVersion > 0 ? config.strategyVersion : 1;
    runRecord.catalogStrategyId   = config.catalogStrategyId;
    runRecord.catalogVersionId    = config.catalogVersionId;
    persistRunRecord(runRecord);

    // Transition to Running
    updateRunStatus(QString(Backtest::Status::Running), QString(), 0, QString());
    emit statusChanged(QString(Backtest::Status::Running));
    m_elapsed.start();

    // Inline pipeline JSON — parse from stored string
    QJsonObject pipelineJson;
    if (!config.pipelineConfigJson.isEmpty()) {
        pipelineJson = QJsonDocument::fromJson(
            config.pipelineConfigJson.toUtf8()).object();
    }

    // Resolve symbols from the pipeline selection config if the run config
    // has no explicit symbols. This ensures data fetching matches the pipeline.
    BacktestRunConfig resolvedConfig = config;
    if (resolvedConfig.symbols.isEmpty() && !pipelineJson.isEmpty()) {
        auto resolved = Pipeline::UniverseResolver::resolve(pipelineJson);
        if (resolved.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols) {
            for (const auto& sym : resolved.symbols)
                resolvedConfig.symbols.append(sym);
            qCDebug(lcBacktestController) << "BacktestController: resolved symbols from pipeline config:"
                     << resolvedConfig.symbols;
        }
    }

    // Build BacktestConfig from the (possibly enriched) run config
    BacktestConfig btConfig = buildBacktestConfig(resolvedConfig);

    // Run on a background thread: HistoricalDataManager + BacktestSession
    m_workerThread = new QThread();

    // We cannot use HistoricalDataManager on the main thread with an existing
    // DB connection (QSqlDatabase is not thread-safe across threads). Instead,
    // open a separate connection on the worker thread using a unique name.
    const QString workerConnName = m_currentRunId + "_worker";

    m_session = new BacktestSession(btConfig);
    m_session->setPipelineConfig(pipelineJson);

    // Connect session signals back to controller (via QueuedConnection — cross-thread)
    connect(m_session, &BacktestSession::progressChanged,
            this, &BacktestController::onSessionProgress,
            Qt::QueuedConnection);
    connect(m_session, &BacktestSession::finished,
            this, &BacktestController::onSessionFinished,
            Qt::QueuedConnection);
    connect(m_session, &BacktestSession::failed,
            this, &BacktestController::onSessionFailed,
            Qt::QueuedConnection);

    // Pre-read the database filename on the main thread (thread-safe).
    // QSqlDatabase::database() is safe to call on the same thread that opened the connection.
    const QString dbFileName = QSqlDatabase::database(m_dbConnectionName).databaseName();

    // Worker function: runs on the background thread
    const QString dataSourceId     = config.dataSourceId;
    const QString resolution       = config.resolution;
    QNetworkAccessManager* netMgr  = m_networkManager;
    BacktestController* self       = this;
    QStringList symbols            = config.symbols;
    QDateTime   startDate          = config.startDate;
    QDateTime   endDate            = config.endDate;

    const QString benchmarkSymbol = config.benchmarkSymbol;

    connect(m_workerThread, &QThread::started, m_session, [=]() mutable {
        // Scope the QSqlDatabase handle so no instance outlives removeDatabase (Qt requirement).
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", workerConnName);
            db.setDatabaseName(dbFileName);
            const bool dbOk = db.open();
            if (!dbOk) {
                qCWarning(lcBacktestController) << "BacktestController worker: cannot open DB — fetching without cache";
            }

            // Use HistoricalDataManager to fetch and cache strategy bars.
            // Pass the bars directly to BacktestSession to eliminate the double-fetch.
            if (dbOk && dataSourceId == QLatin1String("yahoo")) {
                HistoricalDataManager mgr(workerConnName, netMgr);
                QString refreshedAt;
                QMap<QString, QVector<IBComm::HistoricalBar>> strategyBars =
                    mgr.getBarsMulti(symbols, resolution, dataSourceId,
                                     startDate, endDate, &refreshedAt);

                // Inject pre-fetched strategy bars — BacktestSession will skip its own fetch
                m_session->setPreloadedBars(strategyBars);

                // Pre-fetch and inject benchmark bars too (avoid second network round-trip)
                if (!benchmarkSymbol.isEmpty()) {
                    QMap<QString, QVector<IBComm::HistoricalBar>> bmMap =
                        mgr.getBarsMulti({benchmarkSymbol}, QStringLiteral("Day1"),
                                         dataSourceId, startDate, endDate, nullptr);
                    m_session->setPreloadedBenchmarkBars(bmMap.value(benchmarkSymbol));
                }

                // Build QMap<QString, QList<DbHistoricalBar>> for the candlestick UI and
                // relay both refreshedAt and bars to main thread for display after the run.
                QMap<QString, QList<DbHistoricalBar>> uiBars;
                for (auto it = strategyBars.begin(); it != strategyBars.end(); ++it) {
                    QList<DbHistoricalBar> dbList;
                    for (const auto& bar : it.value()) {
                        DbHistoricalBar db;
                        db.symbol       = bar.symbol;
                        db.resolution   = resolution;
                        db.dataSourceId = dataSourceId;
                        db.timestamp    = bar.timestamp.toUTC().toString(Qt::ISODate);
                        db.open         = bar.open;
                        db.high         = bar.high;
                        db.low          = bar.low;
                        db.close        = bar.close;
                        db.volume       = bar.volume;
                        dbList.append(db);
                    }
                    uiBars[it.key()] = dbList;
                }
                QMetaObject::invokeMethod(self, [self, refreshedAt, uiBars]() {
                    self->m_dataRefreshedAt = refreshedAt;
                    self->m_lastHistBars    = uiBars;
                }, Qt::QueuedConnection);
            }
            // For CSV/JSONL sources, BacktestSession handles loading itself (no network)

            // Run the simulation
            m_session->run();

            if (db.isOpen())
                db.close();
        }
        if (QSqlDatabase::contains(workerConnName))
            QSqlDatabase::removeDatabase(workerConnName);
    });

    m_session->moveToThread(m_workerThread);

    m_workerThread->start();
}

// ---------------------------------------------------------------------------
// Private slots — called via QueuedConnection from worker thread
// ---------------------------------------------------------------------------

void BacktestController::onSessionProgress(int percent) {
    emit progressChanged(percent);
}

void BacktestController::onSessionFinished(const BacktestResult& result) {
    const qint64 durationMs = m_elapsed.elapsed();
    persistResult(result, m_dataRefreshedAt);
    updateRunStatus(QString(Backtest::Status::Finished), QString(), durationMs, m_dataRefreshedAt);
    emit statusChanged(QString(Backtest::Status::Finished));

    BacktestLoadedRun loaded;
    loaded.record.runId               = m_currentRunId;
    loaded.record.strategyId =
        Persistence::resolvedStrategyIdForPersistence(m_currentConfig, m_currentRunId);
    loaded.record.strategyDisplayName = m_currentConfig.strategyDisplayName;
    loaded.record.portfolioPath       = m_currentConfig.portfolioPath;
    loaded.record.symbols             = m_currentConfig.symbolsJoined();
    loaded.record.startDate           = m_currentConfig.startDate.toUTC().toString(Qt::ISODate);
    loaded.record.endDate             = m_currentConfig.endDate.toUTC().toString(Qt::ISODate);
    loaded.record.status              = QString(Backtest::Status::Finished);
    loaded.record.durationMs          = durationMs;
    loaded.record.dataRefreshedAt     = m_dataRefreshedAt;
    loaded.record.strategyDefId       = m_currentConfig.strategyDefId;
    loaded.record.strategyVersion     = m_currentConfig.strategyVersion;
    loaded.record.scopeType           = m_currentConfig.scopeType;
    loaded.record.scopeRefId          = m_currentConfig.scopeRefId;
    loaded.record.catalogStrategyId   = m_currentConfig.catalogStrategyId;
    loaded.record.catalogVersionId    = m_currentConfig.catalogVersionId;
    loaded.record.configJson          = QString::fromUtf8(
        QJsonDocument(m_currentConfig.toJson()).toJson(QJsonDocument::Compact));
    loaded.record.createdAt           = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    loaded.result                     = result;
    loaded.histBars                   = m_lastHistBars;

    cleanupWorker();
    m_lastHistBars.clear();

    emit finished(loaded);
}

void BacktestController::onSessionFailed(const QString& reason) {
    const qint64 durationMs = m_elapsed.elapsed();
    updateRunStatus(QString(Backtest::Status::Failed), reason, durationMs, m_dataRefreshedAt);
    emit statusChanged(QString(Backtest::Status::Failed));

    cleanupWorker();

    emit failed(reason);
}

void BacktestController::cleanupWorker() {
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(Backtest::kThreadShutdownTimeoutMs);
        delete m_session;
        m_session = nullptr;
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Persistence helpers
// ---------------------------------------------------------------------------

void BacktestController::persistRunRecord(const DbBacktestRun& run) {
    // Direct synchronous SQL on the main thread using the shared DB connection
    QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
    if (!db.isOpen()) return;

    auto q = query_insertBacktestRun(run, m_dbConnectionName);
    if (!q.exec())
        qCWarning(lcBacktestController) << "BacktestController: persistRunRecord failed:" << q.lastError().text();
}

void BacktestController::updateRunStatus(const QString& status,
                                          const QString& errorText,
                                          qint64 durationMs,
                                          const QString& dataRefreshedAt) {
    QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
    if (!db.isOpen()) return;

    auto q = query_updateBacktestRunStatus(
        m_currentRunId, status, errorText, durationMs, dataRefreshedAt, m_dbConnectionName);
    if (!q.exec())
        qCWarning(lcBacktestController) << "BacktestController: updateRunStatus failed:" << q.lastError().text();
}

void BacktestController::persistResult(const BacktestResult& result,
                                        const QString& dataRefreshedAt) {
    Q_UNUSED(dataRefreshedAt)
    QSqlDatabase db = QSqlDatabase::database(m_dbConnectionName);
    if (!db.isOpen()) return;

    // Metrics
    DbBacktestMetrics m;
    m.runId            = m_currentRunId;
    m.totalReturn      = result.totalReturn;
    m.annualizedReturn = result.annualizedReturn;
    m.sharpeRatio      = result.sharpeRatio;
    m.maxDrawdown      = result.maxDrawdown;
    m.winRate          = result.winRate;
    m.totalTrades      = result.totalTrades;
    m.initialCapital   = result.initialCapital;
    m.finalCapital     = result.finalCapital;
    m.benchmarkReturn  = result.benchmark.totalReturn;
    m.benchmarkSharpe  = result.benchmark.sharpeRatio;
    m.alpha            = result.alphaVsBenchmark;
    {
        auto q = query_insertBacktestMetrics(m, m_dbConnectionName);
        if (!q.exec())
            qCWarning(lcBacktestController) << "BacktestController: persistMetrics failed:" << q.lastError().text();
    }

    // Trades (batch insert in a transaction)
    if (!result.tradeLog.isEmpty()) {
        db.transaction();
        for (const auto& fill : result.tradeLog) {
            DbBacktestTrade t;
            t.runId     = m_currentRunId;
            t.symbol    = fill.symbol;
            t.side      = fill.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
            t.quantity  = std::abs(fill.quantity);
            t.fillPrice = fill.fillPrice;
            t.timestamp = fill.timestamp.toUTC().toString(Qt::ISODate);
            auto q = query_insertBacktestTrade(t, m_dbConnectionName);
            if (!q.exec())
                qCWarning(lcBacktestController) << "BacktestController: insertTrade failed:" << q.lastError().text();
        }
        db.commit();
    }

    // Equity curve — downsample to at most 500 points
    const auto& curve = result.equityCurve;
    const auto& bmCurve = result.benchmark.equityCurve;

    const int total = curve.size();
    const int step  = qMax(1, total / 500);

    if (total > 0) {
        db.transaction();
        for (int i = 0; i < total; i += step) {
            DbBacktestEquityPoint p;
            p.runId          = m_currentRunId;
            p.timestamp      = curve[i].timestamp.toUTC().toString(Qt::ISODate);
            p.value          = curve[i].portfolioValue;
            p.benchmarkValue = (i < bmCurve.size()) ? bmCurve[i].portfolioValue : 0.0;
            auto q = query_insertEquityPoint(p, m_dbConnectionName);
            if (!q.exec())
                qCWarning(lcBacktestController) << "BacktestController: insertEquity failed:" << q.lastError().text();
        }
        db.commit();
    }
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

BacktestConfig BacktestController::buildBacktestConfig(const BacktestRunConfig& rc) {
    BacktestConfig c;
    c.symbols     = rc.symbols;
    c.startDate   = rc.startDate;
    c.endDate     = rc.endDate;
    c.initialCapital   = rc.initialCapital;
    c.benchmarkSymbol  = rc.benchmarkSymbol;
    c.dataSourceId     = rc.dataSourceId;
    c.dataPath         = rc.dataPath;
    c.slippageBps      = rc.slippageBps;

    // Map resolution string to enum
    if      (rc.resolution == "Day1")   c.resolution = BarResolution::Day1;
    else if (rc.resolution == "Hour1")  c.resolution = BarResolution::Hour1;
    else if (rc.resolution == "Min30")  c.resolution = BarResolution::Min30;
    else if (rc.resolution == "Min15")  c.resolution = BarResolution::Min15;
    else if (rc.resolution == "Min5")   c.resolution = BarResolution::Min5;
    else if (rc.resolution == "Min1")   c.resolution = BarResolution::Min1;
    else if (rc.resolution == "Sec5")   c.resolution = BarResolution::Sec5;
    else if (rc.resolution == "Tick")   c.resolution = BarResolution::Tick;
    else                                c.resolution = BarResolution::Day1;

    // Map fillModel string to enum
    if      (rc.fillModel == "Instant")    c.fillModel = FillModelType::Instant;
    else if (rc.fillModel == "MidPrice")   c.fillModel = FillModelType::MidPrice;
    else if (rc.fillModel == "SlippageBps") c.fillModel = FillModelType::SlippageBps;
    else                                    c.fillModel = FillModelType::BidAsk;

    // Map fillTiming string to enum
    if (rc.fillTiming == "SignalOnTick_FillAtBidAsk")
        c.fillTiming = FillTiming::SignalOnTick_FillAtBidAsk;
    else if (rc.fillTiming == "SignalOnClose_FillAtClose")
        c.fillTiming = FillTiming::SignalOnClose_FillAtClose;
    else
        c.fillTiming = FillTiming::SignalOnClose_FillNextBarOpen;

    return c;
}

QString BacktestController::engineVersion() {
    return QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("dev")
        : QCoreApplication::applicationVersion();
}

} // namespace Backtest
