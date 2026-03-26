#ifndef TST_BACKTEST_WORKSPACE_COORDINATOR_H
#define TST_BACKTEST_WORKSPACE_COORDINATOR_H

#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "Common/NHelper.h"
#include "Common/StorageConfig.h"
#include "Backend/ModelTreeRepository.h"
#include "Backend/SystemBackendImpl.h"
#include "DB/dbquery.h"
#include "MainSystem/BacktestWorkspaceCoordinator.h"
#include "MainSystem/ibtradesystemview.h"
#include "BacktestUI/BacktestWorkspaceDock.h"
#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "BacktestUI/PreparePreflightDialog.h"
#include "BacktestUI/YahooSymbolCheckDialog.h"

namespace BacktestWorkspaceCoordinatorTestProbe {
void reset();
int displayedResultCount();
int runHistorySetCount();
QString lastDisplayedRunId();
QStringList lastRunHistoryRunIds();
int resultsStaleSetCount();
bool lastResultsStale();
bool isEmptyStateVisible();
int lastDisplayedHistoricalSymbolCount();
}

class TestUnsavedPrompt final : public IUnsavedChangesPrompt
{
public:
    UnsavedPromptChoice nextChoice = UnsavedPromptChoice::Cancel;
    int promptCount = 0;

    UnsavedPromptChoice askSaveDiscardCancel(QWidget*,
                                             const QString&,
                                             const QString&) override
    {
        ++promptCount;
        return nextChoice;
    }
};

class TestBacktestSessionSwitchPrompt final : public IBacktestSessionSwitchPrompt
{
public:
    BacktestSessionSwitchChoice nextChoice = BacktestSessionSwitchChoice::CancelSwitch;
    int promptCount = 0;
    QString lastMessage;

    BacktestSessionSwitchChoice askContinueCancel(QWidget*,
                                                  const QString&,
                                                  const QString& message) override
    {
        ++promptCount;
        lastMessage = message;
        return nextChoice;
    }
};

class TestBacktestWorkspaceCoordinator : public QObject
{
    Q_OBJECT

private:
    struct StrategyHandle {
        QString nodeId;
        QString name;
        QString strategyDefId;
        QString versionId;
        int versionNumber = 0;
    };

    struct VersionInfo {
        QString versionId;
        int versionNumber = 0;
    };

    std::unique_ptr<QTemporaryDir> m_settingsDir;
    QByteArray m_savedSettingsFileEnv;
    bool m_hadSettingsFileEnv = false;

    QString m_backendDbPath;
    QString m_backtestDbPath;
    QString m_appDataDbPath;
    QString m_settingsFilePath;
    QString m_portfolioId;

    std::unique_ptr<ModelTreeRepository> m_repo;
    std::unique_ptr<SystemBackendImpl> m_backend;

    void setupStorageConfig()
    {
        m_settingsDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_settingsDir && m_settingsDir->isValid());

        m_hadSettingsFileEnv = qEnvironmentVariableIsSet("IBTRADE_SETTINGS_FILE");
        m_savedSettingsFileEnv = qgetenv("IBTRADE_SETTINGS_FILE");
        m_settingsFilePath = m_settingsDir->filePath(QStringLiteral("ibtrade.ini"));
        qputenv("IBTRADE_SETTINGS_FILE", m_settingsFilePath.toUtf8());

        NHelper::initSettings();
        StorageConfig cfg = NHelper::getStorageConfig();
        m_backtestDbPath = m_settingsDir->filePath(QStringLiteral("bt_store.sqlite"));
        m_appDataDbPath = m_settingsDir->filePath(QStringLiteral("app_data.sqlite"));
        cfg.backtestStore.path = m_backtestDbPath;
        cfg.appDataStore.path = m_appDataDbPath;
        NHelper::saveStorageConfig(cfg);
    }

    void setupBackend()
    {
        m_backendDbPath = m_settingsDir->filePath(
            QStringLiteral("backend_%1.sqlite")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        m_repo = std::make_unique<ModelTreeRepository>(
            m_backendDbPath,
            QStringLiteral("bt_coord_conn_")
                + QUuid::createUuid().toString(QUuid::WithoutBraces));
        QVERIFY(m_repo->initialize());
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());

        const QString accountId = m_backend->createAccount(QStringLiteral("Test Account"));
        QVERIFY(!accountId.isEmpty());
        m_portfolioId = m_backend->createPortfolio(accountId, QStringLiteral("Test Portfolio"));
        QVERIFY(!m_portfolioId.isEmpty());
    }

    StrategyHandle createStrategy(const QString& name, const QString& mergePolicy)
    {
        StrategyHandle handle;
        handle.nodeId = m_backend->createStrategy(m_portfolioId, ModelType::STRATEGY_PIPELINE);
        if (handle.nodeId.isEmpty()) {
            QTest::qFail("Failed to create test strategy node", __FILE__, __LINE__);
            return {};
        }
        if (!m_backend->renameNode(handle.nodeId, name)) {
            QTest::qFail("Failed to rename test strategy node", __FILE__, __LINE__);
            return {};
        }
        handle.name = name;

        QJsonObject config = m_backend->pipelineConfig(handle.nodeId);
        config.insert(QStringLiteral("mergePolicy"), mergePolicy);
        config.insert(QStringLiteral("alphas"), QJsonArray());
        if (!m_backend->updatePipelineConfig(handle.nodeId, config)) {
            QTest::qFail("Failed to update test pipeline config", __FILE__, __LINE__);
            return {};
        }

        const QJsonObject defJson = m_backend->strategyDefinitionForNode(handle.nodeId);
        if (defJson.isEmpty()) {
            QTest::qFail("Missing strategy definition for test node", __FILE__, __LINE__);
            return {};
        }
        handle.strategyDefId = defJson.value(QStringLiteral("strategyDefId")).toString();
        handle.versionId = defJson.value(QStringLiteral("versionId")).toString();
        handle.versionNumber = defJson.value(QStringLiteral("version")).toInt(1);
        if (handle.strategyDefId.isEmpty() || handle.versionId.isEmpty()) {
            QTest::qFail("Incomplete strategy definition binding for test node", __FILE__, __LINE__);
            return {};
        }
        return handle;
    }

    Backtest::BacktestLoadedRun makeLiveFinishedRun(const StrategyHandle& handle,
                                                    const QString& runId) const
    {
        Backtest::BacktestLoadedRun run;
        Backtest::BacktestRunConfig config;
        config.strategyId = handle.nodeId;
        config.strategyDisplayName = handle.name;
        config.pipelineConfigJson = QString::fromUtf8(
            QJsonDocument(m_backend->pipelineConfig(handle.nodeId))
                .toJson(QJsonDocument::Compact));
        config.strategyDefId = handle.strategyDefId;
        config.strategyVersion = handle.versionNumber;
        config.catalogStrategyId = handle.strategyDefId;
        config.catalogVersionId = handle.versionId;
        config.symbols = { QStringLiteral("AMD"), QStringLiteral("NVDA") };
        config.startDate = QDateTime(QDate(2021, 3, 26), QTime(0, 0), Qt::UTC);
        config.endDate = QDateTime(QDate(2021, 3, 29), QTime(0, 0), Qt::UTC);
        config.initialCapital = 100000.0;
        config.benchmarkSymbol = QStringLiteral("SPY");
        config.resolution = QStringLiteral("Day1");
        config.dataSourceId = QStringLiteral("yahoo");

        run.record.runId = runId;
        run.record.strategyId = handle.nodeId;
        run.record.strategyDisplayName = handle.name;
        run.record.portfolioPath = QStringLiteral("/Test/%1").arg(handle.name);
        run.record.symbols = QStringLiteral("AMD,NVDA");
        run.record.startDate = config.startDate.toString(Qt::ISODate);
        run.record.endDate = config.endDate.toString(Qt::ISODate);
        run.record.status = QStringLiteral("Finished");
        run.record.dataSourceId = config.dataSourceId;
        run.record.createdAt = QDateTime(QDate(2021, 3, 30), QTime(12, 0), Qt::UTC)
                                   .toString(Qt::ISODate);
        run.record.strategyDefId = handle.strategyDefId;
        run.record.scopeType = QStringLiteral("strategy");
        run.record.scopeRefId = handle.strategyDefId;
        run.record.strategyVersion = handle.versionNumber;
        run.record.catalogStrategyId = handle.strategyDefId;
        run.record.catalogVersionId = handle.versionId;
        run.record.configJson = QString::fromUtf8(
            QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact));
        run.result.startDate = config.startDate;
        run.result.endDate = config.endDate;
        run.result.initialCapital = config.initialCapital;
        run.result.finalCapital = 110000.0;
        run.result.totalReturn = 0.10;
        run.result.annualizedReturn = 0.12;
        run.result.sharpeRatio = 1.4;
        run.result.maxDrawdown = 0.05;
        run.result.winRate = 0.5;
        run.result.totalTrades = 2;
        run.result.alphaVsBenchmark = 0.03;
        run.result.benchmark.symbol = config.benchmarkSymbol;
        run.result.benchmark.totalReturn = 0.07;
        run.result.equityCurve.append(
            {config.startDate, config.initialCapital});
        run.result.equityCurve.append(
            {config.endDate, run.result.finalCapital});
        Backtest::FilledOrder buy;
        buy.orderId = 1;
        buy.symbol = QStringLiteral("AMD");
        buy.quantity = 10.0;
        buy.fillPrice = 100.0;
        buy.timestamp = QDateTime(QDate(2021, 3, 26), QTime(21, 0), Qt::UTC);
        run.result.tradeLog.append(buy);
        Backtest::FilledOrder sell;
        sell.orderId = 2;
        sell.symbol = QStringLiteral("AMD");
        sell.quantity = -10.0;
        sell.fillPrice = 110.0;
        sell.timestamp = QDateTime(QDate(2021, 3, 29), QTime(21, 0), Qt::UTC);
        run.result.tradeLog.append(sell);
        return run;
    }

    Backtest::BacktestLoadedRun makeCatalogFinishedRun(const StrategyHandle& handle,
                                                       const QString& runId) const
    {
        Backtest::BacktestLoadedRun run = makeLiveFinishedRun(handle, runId);
        Backtest::BacktestRunConfig config = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(run.record.configJson.toUtf8()).object());
        config.strategyId.clear();
        run.record.strategyId = handle.strategyDefId;
        run.record.configJson = QString::fromUtf8(
            QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact));
        return run;
    }

    void openStrategy(BacktestWorkspaceCoordinator& coordinator, const StrategyHandle& handle)
    {
        coordinator.openStrategy(handle.nodeId,
                                 handle.name,
                                 QStringLiteral("/Test/%1").arg(handle.name),
                                 m_backend->pipelineConfig(handle.nodeId));
    }

    static bool invokePipelineEdited(BacktestWorkspaceCoordinator& coordinator,
                                     const QJsonObject& pipeline)
    {
        return QMetaObject::invokeMethod(
            &coordinator,
            "onUserPipelineEdited",
            Qt::DirectConnection,
            Q_ARG(QJsonObject, pipeline));
    }

    static bool invokeBacktestFinished(BacktestWorkspaceCoordinator& coordinator,
                                       const Backtest::BacktestLoadedRun& run)
    {
        return QMetaObject::invokeMethod(
            &coordinator,
            "onBacktestFinished",
            Qt::DirectConnection,
            Q_ARG(Backtest::BacktestLoadedRun, run));
    }

    static bool invokeResetToBaseline(BacktestWorkspaceCoordinator& coordinator)
    {
        return QMetaObject::invokeMethod(
            &coordinator,
            "onResetToBaselineRequested",
            Qt::DirectConnection);
    }

    static bool invokeLoadRun(BacktestWorkspaceCoordinator& coordinator,
                              const QString& runId)
    {
        return QMetaObject::invokeMethod(
            &coordinator,
            "onLoadRun",
            Qt::DirectConnection,
            Q_ARG(QString, runId));
    }

    static bool initBacktestStoreSchema(const QString& dbPath)
    {
        const QString conn =
            QStringLiteral("bt_ws_init_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            db.setDatabaseName(dbPath);
            if (!db.open())
                return false;

            auto exec = [&db](const char* sql) {
                QSqlQuery q(db);
                return q.exec(QLatin1String(sql));
            };
            auto execSilent = [&db](const char* sql) {
                QSqlQuery q(db);
                q.exec(QLatin1String(sql));
            };

            if (!exec(CREATE_TABLE_BACKTEST_RUNS))
                return false;
            if (!exec(CREATE_TABLE_BACKTEST_METRICS))
                return false;
            execSilent(ALTER_BACKTEST_METRICS_ADD_SORTINO_RATIO);
            execSilent(ALTER_BACKTEST_METRICS_ADD_CALMAR_RATIO);
            execSilent(ALTER_BACKTEST_METRICS_ADD_PROFIT_FACTOR);
            execSilent(ALTER_BACKTEST_METRICS_ADD_AVERAGE_EXPOSURE_PCT);
            execSilent(ALTER_BACKTEST_METRICS_ADD_TURNOVER_ANNUALIZED);
            execSilent(ALTER_BACKTEST_METRICS_ADD_METRIC_DEFS_VERSION);
            execSilent(ALTER_BACKTEST_METRICS_ADD_STATISTICS_JSON);
            execSilent(ALTER_BACKTEST_METRICS_ADD_BENCHMARK_SYMBOL);
            execSilent(ALTER_BACKTEST_METRICS_ADD_BENCHMARK_ANNUALIZED);
            execSilent(ALTER_BACKTEST_METRICS_ADD_BENCHMARK_MAX_DD);
            execSilent(ALTER_BACKTEST_METRICS_ADD_BENCHMARK_START_PRICE);
            execSilent(ALTER_BACKTEST_METRICS_ADD_BENCHMARK_END_PRICE);
            if (!exec(CREATE_TABLE_BACKTEST_TRADES))
                return false;
            if (!exec(CREATE_TABLE_BACKTEST_EQUITY_CURVE))
                return false;
            if (!exec(CREATE_TABLE_HISTORICAL_BARS))
                return false;
            execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_DEF_ID);
            execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_TYPE);
            execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_REF_ID);
            execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_VERSION);
            execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_STRATEGY_ID);
            execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_VERSION_ID);
            db.close();
        }

        QSqlDatabase::removeDatabase(conn);
        return true;
    }

    void seedPersistedRun(const Backtest::BacktestLoadedRun& run,
                          const QList<DbHistoricalBar>& historicalBars = {})
    {
        QVERIFY(initBacktestStoreSchema(m_backtestDbPath));

        const QString conn =
            QStringLiteral("bt_ws_seed_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            db.setDatabaseName(m_backtestDbPath);
            QVERIFY(db.open());

            DbBacktestRun dbRun;
            dbRun.runId = run.record.runId;
            dbRun.strategyId = run.record.strategyId;
            dbRun.strategyDisplayName = run.record.strategyDisplayName;
            dbRun.portfolioPath = run.record.portfolioPath;
            dbRun.configJson = run.record.configJson;
            dbRun.symbols = run.record.symbols;
            dbRun.startDate = run.record.startDate;
            dbRun.endDate = run.record.endDate;
            dbRun.status = run.record.status;
            dbRun.errorText = run.record.errorText;
            dbRun.durationMs = run.record.durationMs;
            dbRun.engineVersion = run.record.engineVersion;
            dbRun.dataSourceId = run.record.dataSourceId;
            dbRun.dataRefreshedAt = run.record.dataRefreshedAt;
            dbRun.createdAt = run.record.createdAt;
            dbRun.strategyDefId = run.record.strategyDefId;
            dbRun.scopeType = run.record.scopeType;
            dbRun.scopeRefId = run.record.scopeRefId;
            dbRun.strategyVersion = run.record.strategyVersion;
            dbRun.catalogStrategyId = run.record.catalogStrategyId;
            dbRun.catalogVersionId = run.record.catalogVersionId;
            QVERIFY(query_insertBacktestRun(dbRun, conn).exec());

            DbBacktestMetrics metrics;
            metrics.runId = run.record.runId;
            metrics.totalReturn = run.result.totalReturn;
            metrics.annualizedReturn = run.result.annualizedReturn;
            metrics.sharpeRatio = run.result.sharpeRatio;
            metrics.maxDrawdown = run.result.maxDrawdown;
            metrics.winRate = run.result.winRate;
            metrics.totalTrades = run.result.totalTrades;
            metrics.initialCapital = run.result.initialCapital;
            metrics.finalCapital = run.result.finalCapital;
            metrics.benchmarkReturn = run.result.benchmark.totalReturn;
            metrics.benchmarkSharpe = run.result.benchmark.sharpeRatio;
            metrics.benchmarkSymbol = run.result.benchmark.symbol;
            metrics.benchmarkAnnualizedReturn = run.result.benchmark.annualizedReturn;
            metrics.benchmarkMaxDrawdown = run.result.benchmark.maxDrawdown;
            metrics.benchmarkStartPrice = run.result.benchmark.startPrice;
            metrics.benchmarkEndPrice = run.result.benchmark.endPrice;
            metrics.alpha = run.result.alphaVsBenchmark;
            QVERIFY(query_insertBacktestMetrics(metrics, conn).exec());

            for (const Backtest::LedgerSnapshot& point : run.result.equityCurve) {
                DbBacktestEquityPoint dbPoint;
                dbPoint.runId = run.record.runId;
                dbPoint.timestamp = point.timestamp.toUTC().toString(Qt::ISODate);
                dbPoint.value = point.portfolioValue;
                dbPoint.benchmarkValue = 0.0;
                QVERIFY(query_insertEquityPoint(dbPoint, conn).exec());
            }

            for (const Backtest::FilledOrder& trade : run.result.tradeLog) {
                DbBacktestTrade dbTrade;
                dbTrade.runId = run.record.runId;
                dbTrade.symbol = trade.symbol;
                dbTrade.side = trade.quantity >= 0.0
                    ? QStringLiteral("BUY")
                    : QStringLiteral("SELL");
                dbTrade.quantity = qAbs(trade.quantity);
                dbTrade.fillPrice = trade.fillPrice;
                dbTrade.timestamp = trade.timestamp.toUTC().toString(Qt::ISODate);
                QVERIFY(query_insertBacktestTrade(dbTrade, conn).exec());
            }

            for (const DbHistoricalBar& bar : historicalBars)
                QVERIFY(query_upsertHistoricalBar(bar, conn).exec());

            db.close();
        }

        QSqlDatabase::removeDatabase(conn);
    }

    VersionInfo latestVersion(const QString& strategyDefId) const
    {
        const QJsonArray versions = m_backend->listStrategyVersions(strategyDefId);
        if (versions.isEmpty())
            return {};
        const QJsonObject latest = versions.last().toObject();
        VersionInfo info;
        info.versionId = latest.value(QStringLiteral("versionId")).toString();
        info.versionNumber = latest.value(QStringLiteral("versionNumber")).toInt(0);
        return info;
    }

    static QJsonObject currentMergedPipeline(BacktestUI::BacktestWorkspaceDock& dock)
    {
        return QJsonDocument::fromJson(
            dock.runConfigPanel()->mergedPipelineConfigJson().toUtf8()).object();
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<Backtest::BacktestLoadedRun>("Backtest::BacktestLoadedRun");
        QLoggingCategory::setFilterRules(
            QStringLiteral("db.handler.debug=false\nqt.sql.qsqlquery.warning=false"));
    }

    void init()
    {
        BacktestWorkspaceCoordinatorTestProbe::reset();
        setupStorageConfig();
        setupBackend();
    }

    void cleanup()
    {
        m_backend.reset();
        m_repo.reset();

        QFile::remove(m_backendDbPath);
        QFile::remove(m_backtestDbPath);
        QFile::remove(m_backtestDbPath + QStringLiteral("-wal"));
        QFile::remove(m_backtestDbPath + QStringLiteral("-shm"));
        QFile::remove(m_appDataDbPath);
        QFile::remove(m_appDataDbPath + QStringLiteral("-wal"));
        QFile::remove(m_appDataDbPath + QStringLiteral("-shm"));
        QFile::remove(m_settingsFilePath);

        if (m_hadSettingsFileEnv)
            qputenv("IBTRADE_SETTINGS_FILE", m_savedSettingsFileEnv);
        else
            qunsetenv("IBTRADE_SETTINGS_FILE");
        m_settingsDir.reset();
        BacktestWorkspaceCoordinatorTestProbe::reset();
    }

    void testBacktestFinishedDoesNotAutoCreateVersion()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        openStrategy(coordinator, strategy);

        const int versionCountBefore =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        QVERIFY(invokeBacktestFinished(
            coordinator, makeLiveFinishedRun(strategy, QStringLiteral("run-auto"))));
        const int versionCountAfter =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();

        QCOMPARE(versionCountAfter, versionCountBefore);
    }

    void testCoordinatorStartsInEmptyStateWithoutActiveSession()
    {
        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;

        coordinator.setDock(&dock);

        QVERIFY(BacktestWorkspaceCoordinatorTestProbe::isEmptyStateVisible());
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 0);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::runHistorySetCount(), 0);
    }

    void testOpeningStrategyExitsEmptyState()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        QVERIFY(BacktestWorkspaceCoordinatorTestProbe::isEmptyStateVisible());
        openStrategy(coordinator, strategy);

        QVERIFY(!BacktestWorkspaceCoordinatorTestProbe::isEmptyStateVisible());
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategy.nodeId);
    }

    void testSelectingStrategyPopulatesHistoryWithoutAutoLoadingLastRun()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        Backtest::BacktestLoadedRun persisted =
            makeLiveFinishedRun(strategy, QStringLiteral("run-history-only"));
        seedPersistedRun(persisted);

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        openStrategy(coordinator, strategy);

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 0);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedRunId(), QString());
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::runHistorySetCount(), 1);
    }

    void testLiveNodeDirtyAfterRunStillAllowsSaveOnSwitch()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        unsavedPtr->nextChoice = UnsavedPromptChoice::Save;

        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        openStrategy(coordinator, strategyA);
        QVERIFY(invokeBacktestFinished(
            coordinator, makeLiveFinishedRun(strategyA, QStringLiteral("run-save"))));

        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("edited-live-save"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        openStrategy(coordinator, strategyB);

        QCOMPARE(unsavedPtr->promptCount, 1);
        QCOMPARE(switchPtr->promptCount, 0);
        QCOMPARE(
            m_backend->pipelineConfig(strategyA.nodeId).value(QStringLiteral("mergePolicy")).toString(),
            QStringLiteral("edited-live-save"));
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategyB.nodeId);
    }

    void testLiveNodeDirtyAfterRunDiscardStillSwitchesWithoutSaving()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        unsavedPtr->nextChoice = UnsavedPromptChoice::Discard;

        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        openStrategy(coordinator, strategyA);
        QVERIFY(invokeBacktestFinished(
            coordinator, makeLiveFinishedRun(strategyA, QStringLiteral("run-discard"))));

        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("discarded-live-edit"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        openStrategy(coordinator, strategyB);

        QCOMPARE(unsavedPtr->promptCount, 1);
        QCOMPARE(switchPtr->promptCount, 0);
        QCOMPARE(
            m_backend->pipelineConfig(strategyA.nodeId).value(QStringLiteral("mergePolicy")).toString(),
            QStringLiteral("sum"));
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategyB.nodeId);
    }

    void testCatalogPreviewBeforeRunUsesTemporaryWarning()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();
        switchPtr->nextChoice = BacktestSessionSwitchChoice::ContinueSwitch;

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        coordinator.openCatalogVersion(strategyA.strategyDefId, strategyA.versionId);

        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("catalog-before-run"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        openStrategy(coordinator, strategyB);

        QCOMPARE(switchPtr->promptCount, 1);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QVERIFY(!switchPtr->lastMessage.isEmpty());
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategyB.nodeId);
    }

    void testCatalogPreviewAfterRunContinueDiscardsTemporaryState()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();
        switchPtr->nextChoice = BacktestSessionSwitchChoice::ContinueSwitch;

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        coordinator.openCatalogVersion(strategyA.strategyDefId, strategyA.versionId);
        QVERIFY(invokeBacktestFinished(
            coordinator, makeCatalogFinishedRun(strategyA, QStringLiteral("run-continue"))));

        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("catalog-after-run"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        const int versionCountBefore =
            m_backend->listStrategyVersions(strategyA.strategyDefId).size();
        openStrategy(coordinator, strategyB);

        QCOMPARE(switchPtr->promptCount, 1);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategyB.nodeId);
        QCOMPARE(m_backend->listStrategyVersions(strategyA.strategyDefId).size(), versionCountBefore);
    }

    void testCatalogPreviewFinishedRunUpdatesActiveUi()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        coordinator.openCatalogVersion(strategy.strategyDefId, strategy.versionId);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        QVERIFY(invokeBacktestFinished(
            coordinator, makeCatalogFinishedRun(strategy, QStringLiteral("run-active-catalog"))));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 1);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedRunId(),
                 QStringLiteral("run-active-catalog"));
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::runHistorySetCount(), 1);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::resultsStaleSetCount(), 1);
        QVERIFY(!BacktestWorkspaceCoordinatorTestProbe::lastResultsStale());
    }

    void testCatalogVersionRunHistoryIsScopedPerVersion()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        QJsonObject edited = m_backend->pipelineConfig(strategy.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("version-two"));
        QVERIFY(m_backend->updatePipelineConfig(strategy.nodeId, edited));
        const QString newVersionId =
            m_backend->createStrategyVersion(strategy.strategyDefId, edited, QStringLiteral(""));
        QVERIFY(!newVersionId.isEmpty());
        const VersionInfo version2 = latestVersion(strategy.strategyDefId);

        Backtest::BacktestLoadedRun v1Run =
            makeCatalogFinishedRun(strategy, QStringLiteral("run-v1-only"));
        v1Run.record.createdAt =
            QDateTime(QDate(2021, 3, 30), QTime(12, 0), Qt::UTC).toString(Qt::ISODate);
        seedPersistedRun(v1Run);

        Backtest::BacktestLoadedRun v2Run = v1Run;
        v2Run.record.runId = QStringLiteral("run-v2-only");
        v2Run.record.catalogVersionId = version2.versionId;
        v2Run.record.strategyVersion = version2.versionNumber;
        Backtest::BacktestRunConfig v2Config = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(v2Run.record.configJson.toUtf8()).object());
        v2Config.catalogVersionId = version2.versionId;
        v2Config.strategyVersion = version2.versionNumber;
        v2Run.record.configJson = QString::fromUtf8(
            QJsonDocument(v2Config.toJson()).toJson(QJsonDocument::Compact));
        v2Run.record.createdAt =
            QDateTime(QDate(2021, 3, 31), QTime(12, 0), Qt::UTC).toString(Qt::ISODate);
        seedPersistedRun(v2Run);

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        coordinator.openCatalogVersion(strategy.strategyDefId, strategy.versionId);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastRunHistoryRunIds(),
                 QStringList{QStringLiteral("run-v1-only")});

        coordinator.openCatalogVersion(strategy.strategyDefId, version2.versionId);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastRunHistoryRunIds(),
                 QStringList{QStringLiteral("run-v2-only")});
    }

    void testCatalogPreviewAfterRunCancelKeepsCurrentSession()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();
        switchPtr->nextChoice = BacktestSessionSwitchChoice::CancelSwitch;

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        coordinator.openCatalogVersion(strategyA.strategyDefId, strategyA.versionId);
        QVERIFY(invokeBacktestFinished(
            coordinator, makeCatalogFinishedRun(strategyA, QStringLiteral("run-cancel"))));

        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("catalog-cancel"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        openStrategy(coordinator, strategyB);

        QCOMPARE(switchPtr->promptCount, 1);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QCOMPARE(dock.runConfigPanel()->currentConfig().catalogVersionId, strategyA.versionId);
    }

    void testExplicitSaveAsNewVersionRebasesSessionState()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle other =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        openStrategy(coordinator, strategy);

        QJsonObject edited = m_backend->pipelineConfig(strategy.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("explicit-save"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        QSignalSpy refreshSpy(&coordinator, &BacktestWorkspaceCoordinator::catalogRefreshNeeded);
        QVERIFY(refreshSpy.isValid());

        const int versionCountBefore =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        const auto result = coordinator.saveActiveSessionAsNewVersion();
        const int versionCountAfter =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        const VersionInfo latest = latestVersion(strategy.strategyDefId);

        QCOMPARE(result.outcome,
                 BacktestWorkspaceCoordinator::SaveAsNewVersionResult::Outcome::Created);
        QCOMPARE(versionCountAfter, versionCountBefore + 1);
        QCOMPARE(refreshSpy.count(), 1);
        QCOMPARE(result.version.versionId, latest.versionId);
        QCOMPARE(result.version.versionNumber, latest.versionNumber);
        QCOMPARE(dock.runConfigPanel()->currentConfig().catalogVersionId, latest.versionId);
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyVersion, latest.versionNumber);

        QJsonObject secondEdit = currentMergedPipeline(dock);
        secondEdit.insert(QStringLiteral("mergePolicy"), QStringLiteral("second-edit"));
        QVERIFY(invokePipelineEdited(coordinator, secondEdit));
        QVERIFY(invokeResetToBaseline(coordinator));
        QCOMPARE(currentMergedPipeline(dock).value(QStringLiteral("mergePolicy")).toString(),
                 QStringLiteral("explicit-save"));

        openStrategy(coordinator, other);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QCOMPARE(switchPtr->promptCount, 0);
    }

    void testCatalogPreviewSaveAsNewVersionRekeysSessionState()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle other =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        coordinator.openCatalogVersion(strategy.strategyDefId, strategy.versionId);

        QJsonObject edited = currentMergedPipeline(dock);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("catalog-save"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        QSignalSpy refreshSpy(&coordinator, &BacktestWorkspaceCoordinator::catalogRefreshNeeded);
        QVERIFY(refreshSpy.isValid());

        const int versionCountBefore =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        const auto result = coordinator.saveActiveSessionAsNewVersion();
        const int versionCountAfter =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        const VersionInfo latest = latestVersion(strategy.strategyDefId);

        QCOMPARE(result.outcome,
                 BacktestWorkspaceCoordinator::SaveAsNewVersionResult::Outcome::Created);
        QCOMPARE(versionCountAfter, versionCountBefore + 1);
        QCOMPARE(refreshSpy.count(), 1);
        QCOMPARE(result.version.versionId, latest.versionId);
        QCOMPARE(result.version.versionNumber, latest.versionNumber);
        QCOMPARE(dock.runConfigPanel()->currentConfig().catalogVersionId, latest.versionId);
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyVersion, latest.versionNumber);

        QJsonObject secondEdit = currentMergedPipeline(dock);
        secondEdit.insert(QStringLiteral("mergePolicy"), QStringLiteral("catalog-second-edit"));
        QVERIFY(invokePipelineEdited(coordinator, secondEdit));
        QVERIFY(invokeResetToBaseline(coordinator));
        QCOMPARE(currentMergedPipeline(dock).value(QStringLiteral("mergePolicy")).toString(),
                 QStringLiteral("catalog-save"));

        openStrategy(coordinator, other);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QCOMPARE(switchPtr->promptCount, 0);
    }

    void testExplicitSaveAsNewVersionDuplicateRebindsToExistingVersion()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle other =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        auto unsavedPrompt = std::make_unique<TestUnsavedPrompt>();
        TestUnsavedPrompt* unsavedPtr = unsavedPrompt.get();
        auto switchPrompt = std::make_unique<TestBacktestSessionSwitchPrompt>();
        TestBacktestSessionSwitchPrompt* switchPtr = switchPrompt.get();

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        coordinator.setUnsavedChangesPrompt(std::move(unsavedPrompt));
        coordinator.setBacktestSessionSwitchPrompt(std::move(switchPrompt));

        openStrategy(coordinator, strategy);
        QJsonObject duplicateConfig = currentMergedPipeline(dock);
        duplicateConfig.insert(QStringLiteral("mergePolicy"), QStringLiteral("existing-version"));
        QVERIFY(invokePipelineEdited(coordinator, duplicateConfig));

        duplicateConfig = currentMergedPipeline(dock);
        const QString existingVersionId = m_backend->createStrategyVersion(
            strategy.strategyDefId, duplicateConfig, QStringLiteral("existing"), strategy.versionId);
        QVERIFY(!existingVersionId.isEmpty());
        const VersionInfo existingVersion = latestVersion(strategy.strategyDefId);

        const int versionCountBefore =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();
        const auto result = coordinator.saveActiveSessionAsNewVersion();
        const int versionCountAfter =
            m_backend->listStrategyVersions(strategy.strategyDefId).size();

        QCOMPARE(result.outcome,
                 BacktestWorkspaceCoordinator::SaveAsNewVersionResult::Outcome::AlreadyExists);
        QCOMPARE(versionCountAfter, versionCountBefore);
        QCOMPARE(result.version.versionId, existingVersion.versionId);
        QCOMPARE(result.version.versionNumber, existingVersion.versionNumber);
        QCOMPARE(dock.runConfigPanel()->currentConfig().catalogVersionId, existingVersion.versionId);
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyVersion,
                 existingVersion.versionNumber);

        openStrategy(coordinator, other);
        QCOMPARE(unsavedPtr->promptCount, 0);
        QCOMPARE(switchPtr->promptCount, 0);
    }

    void testInactiveFinishedRunDoesNotOverwriteActiveDisplayOrHistory()
    {
        const StrategyHandle strategyA =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));
        const StrategyHandle strategyB =
            createStrategy(QStringLiteral("Beta"), QStringLiteral("max"));

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);

        openStrategy(coordinator, strategyA);
        openStrategy(coordinator, strategyB);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        QVERIFY(invokeBacktestFinished(
            coordinator, makeLiveFinishedRun(strategyA, QStringLiteral("run-inactive"))));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 0);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::runHistorySetCount(), 0);
        QCOMPARE(dock.runConfigPanel()->currentConfig().strategyId, strategyB.nodeId);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        openStrategy(coordinator, strategyA);
        QJsonObject edited = m_backend->pipelineConfig(strategyA.nodeId);
        edited.insert(QStringLiteral("mergePolicy"), QStringLiteral("inactive-finish-edit"));
        QVERIFY(invokePipelineEdited(coordinator, edited));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::resultsStaleSetCount(), 1);
        QVERIFY(BacktestWorkspaceCoordinatorTestProbe::lastResultsStale());
    }

    void testLoadingHistoricalRunHydratesCachedCandlestickBars()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        Backtest::BacktestLoadedRun persisted =
            makeLiveFinishedRun(strategy, QStringLiteral("run-with-bars"));
        const Backtest::BacktestRunConfig config = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(persisted.record.configJson.toUtf8()).object());

        QList<DbHistoricalBar> bars;
        DbHistoricalBar bar1;
        bar1.symbol = QStringLiteral("AMD");
        bar1.resolution = config.resolution;
        bar1.dataSourceId = config.dataSourceId;
        bar1.timestamp = QDateTime(QDate(2021, 3, 26), QTime(21, 0), Qt::UTC).toString(Qt::ISODate);
        bar1.open = 100.0;
        bar1.high = 101.0;
        bar1.low = 99.0;
        bar1.close = 100.5;
        bar1.volume = 1000.0;
        bars.append(bar1);

        DbHistoricalBar bar2 = bar1;
        bar2.timestamp = QDateTime(QDate(2021, 3, 29), QTime(21, 0), Qt::UTC).toString(Qt::ISODate);
        bar2.close = 110.0;
        bars.append(bar2);

        seedPersistedRun(persisted, bars);

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        openStrategy(coordinator, strategy);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        QVERIFY(invokeLoadRun(coordinator, persisted.record.runId));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 1);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedRunId(),
                 persisted.record.runId);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedHistoricalSymbolCount(), 1);
    }

    void testLoadingHistoricalRunWithoutCachedBarsStillDisplaysResult()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        Backtest::BacktestLoadedRun persisted =
            makeLiveFinishedRun(strategy, QStringLiteral("run-without-bars"));
        seedPersistedRun(persisted);

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        openStrategy(coordinator, strategy);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        QVERIFY(invokeLoadRun(coordinator, persisted.record.runId));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 1);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedHistoricalSymbolCount(), 0);
    }

    void testLoadingHistoricalRunUsesAvailableSymbolsOnly()
    {
        const StrategyHandle strategy =
            createStrategy(QStringLiteral("Alpha"), QStringLiteral("sum"));

        Backtest::BacktestLoadedRun persisted =
            makeLiveFinishedRun(strategy, QStringLiteral("run-partial-bars"));
        const Backtest::BacktestRunConfig config = Backtest::BacktestRunConfig::fromJson(
            QJsonDocument::fromJson(persisted.record.configJson.toUtf8()).object());

        QList<DbHistoricalBar> bars;
        DbHistoricalBar bar;
        bar.symbol = QStringLiteral("NVDA");
        bar.resolution = config.resolution;
        bar.dataSourceId = config.dataSourceId;
        bar.timestamp = QDateTime(QDate(2021, 3, 26), QTime(21, 0), Qt::UTC).toString(Qt::ISODate);
        bar.open = 200.0;
        bar.high = 205.0;
        bar.low = 198.0;
        bar.close = 204.0;
        bar.volume = 1200.0;
        bars.append(bar);

        seedPersistedRun(persisted, bars);

        BacktestWorkspaceCoordinator coordinator;
        coordinator.setBackend(m_backend.get());
        BacktestUI::BacktestWorkspaceDock dock;
        coordinator.setDock(&dock);
        openStrategy(coordinator, strategy);

        BacktestWorkspaceCoordinatorTestProbe::reset();
        QVERIFY(invokeLoadRun(coordinator, persisted.record.runId));

        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::displayedResultCount(), 1);
        QCOMPARE(BacktestWorkspaceCoordinatorTestProbe::lastDisplayedHistoricalSymbolCount(), 1);
    }
};

#endif // TST_BACKTEST_WORKSPACE_COORDINATOR_H
