#include "Backtest/BacktestController.h"
#include "Backtest/BacktestPreFlightCoordinator.h"
#include "Backtest/BacktestMetricsMapper.h"
#include "Backtest/BacktestStatisticsCalculator.h"
#include "Backtest/BacktestRunPersistence.h"
#include "Backtest/HistoricalDataManager.h"
#include "Backtest/BacktestConstants.h"
#include "Backtest/LedgerSnapshot.h"
#include "Pipeline/StrategyPipelineRuntimeOptions.h"
#include "DB/dbquery.h"
#include "DB/dbdatatypes.h"
#include <algorithm>
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QVariantMap>
#include <QMap>
#include <QVector>
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <cmath>
#include <memory>

Q_LOGGING_CATEGORY(lcBacktestController, "backtest.controller")

namespace Backtest {

namespace {

QHash<QString, QVariantMap> assetListJsonToHash(const QString& json)
{
    QHash<QString, QVariantMap> out;
    if (json.trimmed().isEmpty())
        return out;
    QJsonParseError err;
    const QJsonDocument d = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !d.isObject())
        return out;
    const QJsonObject o = d.object();
    for (auto it = o.begin(); it != o.end(); ++it) {
        if (it.value().isObject())
            out.insert(it.key(), it.value().toObject().toVariantMap());
    }
    return out;
}

/// Strategy and benchmark equity series can differ in length (e.g. many strategy bars vs daily
/// benchmark). Map each strategy timestamp to the benchmark portfolio value as-of that time
/// (last benchmark point at or before \a t). Never use index alignment or zeros as placeholders.
double benchmarkPortfolioValueAtTimestamp(const QVector<LedgerSnapshot>& bm,
                                          const QDateTime& t)
{
    if (bm.isEmpty())
        return 0.0;
    if (t <= bm.first().timestamp)
        return bm.first().portfolioValue;
    if (t >= bm.last().timestamp)
        return bm.last().portfolioValue;

    auto it = std::lower_bound(
        bm.begin(), bm.end(), t,
        [](const LedgerSnapshot& s, const QDateTime& tt) { return s.timestamp < tt; });
    if (it == bm.end())
        return bm.last().portfolioValue;
    if (it->timestamp == t)
        return it->portfolioValue;
    if (it == bm.begin())
        return it->portfolioValue;
    return (it - 1)->portfolioValue;
}

/// Persists metrics, trades, and equity points using a dedicated connection (any thread).
void persistBacktestResultImpl(const QString& dbFilePath,
                               const QString& uniqueConnectionName,
                               const QString& runId,
                               const BacktestResult& result)
{
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), uniqueConnectionName);
        db.setDatabaseName(dbFilePath);
        if (!db.open()) {
            qCWarning(lcBacktestController) << "BacktestController: persist thread cannot open DB";
            return;
        }

        BacktestStatisticsCalculator calc;
        const BacktestStatistics     stats = calc.compute(result);
        const DbBacktestMetrics        m   = toDbBacktestMetrics(runId, result, stats);
        {
            auto q = query_insertBacktestMetrics(m, uniqueConnectionName);
            if (!q.exec())
                qCWarning(lcBacktestController) << "BacktestController: persistMetrics failed:" << q.lastError().text();
        }

        if (!result.tradeLog.isEmpty()) {
            db.transaction();
            for (const auto& fill : result.tradeLog) {
                DbBacktestTrade t;
                t.runId     = runId;
                t.symbol    = fill.symbol;
                t.side      = fill.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
                t.quantity  = std::abs(fill.quantity);
                t.fillPrice = fill.fillPrice;
                t.timestamp = fill.timestamp.toUTC().toString(Qt::ISODate);
                auto q = query_insertBacktestTrade(t, uniqueConnectionName);
                if (!q.exec())
                    qCWarning(lcBacktestController) << "BacktestController: insertTrade failed:" << q.lastError().text();
            }
            db.commit();
        }

        const auto& curve   = result.equityCurve;
        const auto& bmCurve = result.benchmark.equityCurve;
        const int total     = curve.size();
        const int step      = qMax(1, total / 500);

        if (total > 0) {
            db.transaction();
            for (int i = 0; i < total; i += step) {
                DbBacktestEquityPoint p;
                p.runId          = runId;
                p.timestamp      = curve[i].timestamp.toUTC().toString(Qt::ISODate);
                p.value          = curve[i].portfolioValue;
                p.benchmarkValue = benchmarkPortfolioValueAtTimestamp(bmCurve, curve[i].timestamp);
                auto q = query_insertEquityPoint(p, uniqueConnectionName);
                if (!q.exec())
                    qCWarning(lcBacktestController) << "BacktestController: insertEquity failed:" << q.lastError().text();
            }
            db.commit();
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(uniqueConnectionName);
}

} // namespace

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
    } else {
        m_dbFilePath = db.databaseName();
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

void BacktestController::requestStop()
{
    if (!m_session || !m_workerThread || !m_workerThread->isRunning())
        return;
    // Must not use QueuedConnection: the worker thread is blocked inside replay() and will not
    // process events until the loop ends. cancel() only sets an atomic flag (thread-safe).
    m_session->cancel();
}

void BacktestController::start(const BacktestRunConfig& config) {
    if (isRunning()) {
        qCWarning(lcBacktestController) << "BacktestController: a run is already in progress — ignoring start()";
        return;
    }

    m_currentRunId  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_dataRefreshedAt.clear();

    const QString persistedStrategyId =
        Persistence::resolvedStrategyIdForPersistence(config, m_currentRunId);

    // Inline pipeline JSON — parse from stored string
    QJsonObject pipelineJson;
    if (!config.pipelineConfigJson.isEmpty()) {
        pipelineJson = QJsonDocument::fromJson(
            config.pipelineConfigJson.toUtf8()).object();
    }

    const Pipeline::StrategyPipelineRuntimeOptions runtimeOpts =
        Pipeline::StrategyPipelineRuntimeOptions::fromJson(pipelineJson);

    // Resolve symbols from the pipeline selection config if the run config
    // has no explicit symbols. This ensures data fetching matches the pipeline.
    BacktestRunConfig resolvedConfig = BacktestPreFlightCoordinator::resolveRunConfigSymbols(config);
    if (resolvedConfig.symbols != config.symbols) {
        qCDebug(lcBacktestController) << "BacktestController: resolved symbols from pipeline config:"
                                      << resolvedConfig.symbols;
    }

    // Authoritative run config for persistence and onSessionFinished (includes resolved symbols).
    m_currentConfig = resolvedConfig;

    // Persist run record with resolved symbols + full configJson (matches actual execution).
    DbBacktestRun runRecord;
    runRecord.runId               = m_currentRunId;
    runRecord.strategyId          = persistedStrategyId;
    runRecord.strategyDisplayName = m_currentConfig.strategyDisplayName;
    runRecord.portfolioPath       = m_currentConfig.portfolioPath;
    runRecord.configJson          = QString::fromUtf8(
        QJsonDocument(m_currentConfig.toJson()).toJson(QJsonDocument::Compact));
    runRecord.symbols             = m_currentConfig.symbolsJoined();
    runRecord.startDate           = m_currentConfig.startDate.toUTC().toString(Qt::ISODate);
    runRecord.endDate             = m_currentConfig.endDate.toUTC().toString(Qt::ISODate);
    runRecord.status              = QString(Backtest::Status::Created);
    runRecord.engineVersion       = engineVersion();
    runRecord.dataSourceId        = m_currentConfig.dataSourceId;
    runRecord.createdAt           = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    // Canonical scope fields — caller is responsible for populating these before calling start().
    // Do NOT set scopeRefId = config.strategyId implicitly (they serve different semantics).
    runRecord.strategyDefId       = m_currentConfig.strategyDefId;
    runRecord.scopeType           = m_currentConfig.scopeType.isEmpty()
                                      ? QString(Backtest::Scope::Strategy)
                                      : m_currentConfig.scopeType;
    runRecord.scopeRefId          = m_currentConfig.scopeRefId;
    runRecord.strategyVersion     = m_currentConfig.strategyVersion > 0 ? m_currentConfig.strategyVersion : 1;
    runRecord.catalogStrategyId   = m_currentConfig.catalogStrategyId;
    runRecord.catalogVersionId    = m_currentConfig.catalogVersionId;
    persistRunRecord(runRecord);

    // Transition to Running
    updateRunStatus(QString(Backtest::Status::Running), QString(), 0, QString());
    emit statusChanged(QString(Backtest::Status::Running));
    m_elapsed.start();

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
    const QString dataSourceId     = resolvedConfig.dataSourceId;
    const QString resolution       = resolvedConfig.resolution;
    QNetworkAccessManager* netMgr  = m_networkManager;
    BacktestController* self       = this;
    QStringList symbols            = resolvedConfig.symbols;
    QDateTime   startDate          = resolvedConfig.startDate;
    QDateTime   endDate            = resolvedConfig.endDate;

    const QString benchmarkSymbol = resolvedConfig.benchmarkSymbol;
    const QHash<QString, QVariantMap> strategyAssets = assetListJsonToHash(resolvedConfig.assetListJson);
    const Pipeline::HistoricalReadPolicy historicalReadPolicy = runtimeOpts.historicalReadPolicy;

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
            // Keep the manager alive for the whole session so `IHistoricalRead` (semantic alphas)
            // can query the same cache during replay.
            std::unique_ptr<HistoricalDataManager> histMgr;
            if (dbOk && dataSourceId == QLatin1String("yahoo")) {
                histMgr = std::make_unique<HistoricalDataManager>(workerConnName, netMgr);
                QString refreshedAt;
                QMap<QString, QVector<IBComm::HistoricalBar>> strategyBars =
                    histMgr->getBarsMulti(symbols, resolution, dataSourceId,
                                          startDate, endDate, &refreshedAt, strategyAssets, historicalReadPolicy);

                // Inject pre-fetched strategy bars — BacktestSession will skip its own fetch
                m_session->setPreloadedBars(strategyBars);
                m_session->setHistoricalDataManager(histMgr.get());
                m_session->setHistoricalReadPolicy(historicalReadPolicy);

                // Pre-fetch and inject benchmark bars too (avoid second network round-trip)
                if (!benchmarkSymbol.isEmpty()) {
                    QMap<QString, QVector<IBComm::HistoricalBar>> bmMap =
                        histMgr->getBarsMulti({benchmarkSymbol}, QStringLiteral("Day1"),
                                              dataSourceId, startDate, endDate, nullptr, strategyAssets, historicalReadPolicy);
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
            } else {
                m_session->setHistoricalDataManager(nullptr);
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

    if (m_dbFilePath.isEmpty()) {
        updateRunStatus(QString(Backtest::Status::Finished), QString(), durationMs, m_dataRefreshedAt);
        emit statusChanged(QString(Backtest::Status::Finished));
        emit finished(loaded);
        return;
    }

    const QString persistConn =
        QStringLiteral("bt_persist_") + m_currentRunId + QLatin1Char('_') +
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString runId     = m_currentRunId;
    const QString dbFile    = m_dbFilePath;
    const QString dataRef   = m_dataRefreshedAt;
    const BacktestResult resCopy = result;

    auto* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this,
            [this, loaded, watcher, durationMs, dataRef]() {
                watcher->deleteLater();
                updateRunStatus(QString(Backtest::Status::Finished), QString(), durationMs, dataRef);
                emit statusChanged(QString(Backtest::Status::Finished));
                emit finished(loaded);
            });
    watcher->setFuture(QtConcurrent::run([dbFile, persistConn, runId, resCopy]() {
        persistBacktestResultImpl(dbFile, persistConn, runId, resCopy);
    }));
}

void BacktestController::onSessionFailed(const QString& reason) {
    const qint64 durationMs = m_elapsed.elapsed();
    const bool cancelled =
        reason.startsWith(QStringLiteral("Cancelled"));
    if (cancelled) {
        updateRunStatus(QString(Backtest::Status::Cancelled), reason, durationMs, m_dataRefreshedAt);
        emit statusChanged(QString(Backtest::Status::Cancelled));
    } else {
        updateRunStatus(QString(Backtest::Status::Failed), reason, durationMs, m_dataRefreshedAt);
        emit statusChanged(QString(Backtest::Status::Failed));
    }

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
