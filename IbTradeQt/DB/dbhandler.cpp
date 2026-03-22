#include "dbhandler.h"
#include "NHelper.h"
#include "dbquery.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QLoggingCategory>
#include <QUuid>

Q_LOGGING_CATEGORY(lcDbHandler, "db.handler")

DBHandler::DBHandler(QObject *parent) : QObject(parent) {
    // Constructor code
    m_uniqueConnectionName = QUuid::createUuid().toString();
}

DBHandler::~DBHandler() {
    disconnectDB();
}

bool DBHandler::connectDB(const QString& dbName) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_uniqueConnectionName);
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qCDebug(lcDbHandler) << "Error: Connection with database failed";
        return false;
    }

    initializeBacktestTables();
    return true;
}

void DBHandler::initializeBacktestTables() {
    auto exec = [this](const char* sql) {
        QSqlQuery q(m_db);
        if (!q.exec(QLatin1String(sql)))
            qCWarning(lcDbHandler) << "DBHandler: failed to create table:" << q.lastError().text();
    };

    // Silence errors for ALTER TABLE when columns already exist
    auto execSilent = [this](const char* sql) {
        QSqlQuery q(m_db);
        q.exec(QLatin1String(sql)); // intentionally ignore return value
    };

    exec(CREATE_TABLE_BACKTEST_RUNS);
    exec(CREATE_TABLE_BACKTEST_METRICS);
    exec(CREATE_TABLE_BACKTEST_TRADES);
    exec(CREATE_TABLE_BACKTEST_EQUITY_CURVE);
    exec(CREATE_TABLE_HISTORICAL_BARS);
    exec(CREATE_TABLE_BACKTEST_RUN_PROFILES);

    // Extend BacktestRuns with new scope/definition columns (idempotent)
    execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_DEF_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_TYPE);
    execSilent(ALTER_BACKTEST_RUNS_ADD_SCOPE_REF_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_STRATEGY_VERSION);

    // v3 catalog columns (idempotent)
    execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_STRATEGY_ID);
    execSilent(ALTER_BACKTEST_RUNS_ADD_CATALOG_VERSION_ID);
}

void DBHandler::disconnectDB() {
    if (!QSqlDatabase::contains(m_uniqueConnectionName))
        return;

    if (m_db.isOpen())
        m_db.close();

    // QSqlDatabase must not reference this connection name when removeDatabase runs.
    m_db = QSqlDatabase();

    QSqlDatabase::removeDatabase(m_uniqueConnectionName);
}

bool DBHandler::initializeDatabase() {

    auto createTableIfNotExists = [this](const QString& tableName, const QString& creationQueryTemplate) {
        bool success = true;
        QSqlQuery query(m_db);

        if (!m_db.tables().contains(tableName)) {
            success = query.exec(creationQueryTemplate.arg(tableName));
            if (!success) {
                qCDebug(lcDbHandler) << "Failed to create table" << tableName << ":" << query.lastError().text();
            }
        }

        return success;
    };



    // Initialize database tables
    // StrategyData Table
    bool success = createTableIfNotExists(TABLE_STRATEGYDATA, CREATE_TABLE_STRATEGYDATA_TEMPLATE);
    // ModelInfo Table
    success |= createTableIfNotExists(TABLE_MODELINFO, CREATE_TABLE_MODELINFO_TEMPLATE);
    // Trades Table
    success |= createTableIfNotExists(TABLE_TRADES, CREATE_TABLE_TRADES_TEMPLATE);
    // Positions Table
    success |= createTableIfNotExists(TABLE_POSITIONS, CREATE_TABLE_POSITIONS_TEMPLATE);

    success |= createTrigger(m_db);

  return success;
}

bool DBHandler::createTrigger(QSqlDatabase& db) {
    QSqlQuery query(db);
    // QString triggerCommand = R"(
    //     CREATE TRIGGER IF NOT EXISTS update_or_insert_position
    //     AFTER INSERT ON Trades
    //     FOR EACH ROW
    //     BEGIN
    //         -- Check if the trade is a "SELL" trade and adjust the new quantity accordingly
    //         INSERT INTO Positions (strategyId, symbol, quantity, averageOpenPrice, pnl, fee, openDate, status)
    //         VALUES (NEW.strategyId, NEW.symbol,
    //                 CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END,
    //                 NEW.price, NEW.pnl, NEW.fee, NEW.date, 1)
    //         ON CONFLICT (strategyId, symbol)
    //         DO
    //         UPDATE SET quantity = quantity +
    //                 CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END,
    //                pnl = pnl + NEW.pnl,
    //                fee = fee + NEW.fee,
    //                averageOpenPrice = (averageOpenPrice * quantity + NEW.price *
    //                 CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END) /
    //                 (quantity + CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END),
    //                openDate = CASE WHEN openDate IS NULL THEN NEW.date ELSE openDate END,
    //                status = CASE WHEN quantity +
    //                 CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END = 0 THEN 0 ELSE status END;

    //         -- Delete the entry from the Positions table when quantity becomes zero
    //         DELETE FROM Positions WHERE strategyId = NEW.strategyId AND symbol = NEW.symbol AND quantity = 0;
    //     END;

    // )";

    QString triggerCommand =  QString(CREATE_UPDATE_OR_INSERT_TRIGGER_TEMPLATE).arg(TABLE_TRADES, TABLE_POSITIONS);

    if (!query.exec(triggerCommand)) {
        qCDebug(lcDbHandler) << "Error creating trigger:" << query.lastError().text();
        return false;
    }

    return true;
}


void DBHandler::slotAddPositionQuery(const OpenPosition &position)
{
    auto query = query_addCurrentPosition(position, m_uniqueConnectionName);
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error executing query:" << query.lastError();
    } else {
        qCDebug(lcDbHandler) << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotAddNewTrade(const DbTrade &trade)
{
    auto query = query_addNewTrade(trade, m_uniqueConnectionName);
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error executing query:" << query.lastError();
    } else {
        qCDebug(lcDbHandler) << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotUpdateTradeCommission(const DbTradeCommission &tradeComm)
{
    auto query = query_updateTrade(tradeComm, m_uniqueConnectionName);
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error executing query:" << query.lastError();
    } else {
        qCDebug(lcDbHandler) << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }

}

void DBHandler::initializeConnectionSlot()
{
    const QString dbPath = NHelper::getStorageConfig().appDataStore.path;
    bool isConnected = connectDB(dbPath);
    if(isConnected)
        isConnected = initializeDatabase();
    emit signalDBConnectionState(isConnected);
}

void DBHandler::slotFetchOpenPositions(const QString& strategy_id)
{
    QList<OpenPosition> positionsList;
    QSqlQuery query(query_getOpenPositions(strategy_id, m_uniqueConnectionName));
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error fetching open positions:" << query.lastError();
        emit signalOpenPositionsFetched(positionsList, QS_ERROR);
    }
    else
    {
        while (query.next()) {
            OpenPosition position;
            position.strategyId = query.value("strategyId").toString();
            position.symbol = query.value("symbol").toString();
            position.quantity = query.value("quantity").toInt();
            position.price = query.value("averageOpenPrice").toDouble();
            position.pnl = query.value("pnl").toDouble();
            position.fee = query.value("fee").toDouble();
            position.date = query.value("closeDate").toString();
            position.status = query.value("status").toInt();

            positionsList.append(position);
        }
        if(positionsList.length() > 0){
            emit signalOpenPositionsFetched(positionsList, QS_VALID);
        }
        else{
            emit signalOpenPositionsFetched(positionsList, QS_NOT_FOUND);
        }
    }

}

void DBHandler::slotAddOrUpdateDbModelInfo(const DbModelInfo &obj)
{
    auto query = query_addOrUpdateDbModelInfo(obj, m_uniqueConnectionName);
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error executing query:" << query.lastError();
    } else {
        qCDebug(lcDbHandler) << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotGetModelInfo(const QString &modelId)
{
    DbModelInfo obj;
    QSqlQuery query(query_getDbModelInfo(modelId, m_uniqueConnectionName));
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error fetching model info:" << query.lastError();
        emit signalModelInfoFetched(obj, e_queryStatus::QS_ERROR);
    }
    else
    {
        if (query.next()) {  // Assuming only one result per strategy_id
            obj.modelId = query.value("modelId").toString();
            obj.modelName = query.value("modelName").toString();
            obj.modelDescription = query.value("modelDescription").toString();
            obj.createdAt = query.value("createdAt").toDateTime();
            obj.updatedAt = query.value("updatedAt").toDateTime();
            obj.status = query.value("status").toString();

            emit signalModelInfoFetched(obj, e_queryStatus::QS_VALID);
        } else {
            //qDebug() << "No model info found for modelId:" << modelId;
            emit signalModelInfoFetched(obj, e_queryStatus::QS_NOT_FOUND);
        }
    }
}


void DBHandler::slotAddOrUpdateDbStrategyData(const DbStrategyData &obj)
{
    auto query = query_addOrUpdateDbStrategyData(obj, m_uniqueConnectionName);
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error executing query:" << query.lastError();
    } else {
        qCDebug(lcDbHandler) << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotGetStrategyData(const QString &strategy_id)
{
    DbStrategyData obj;
    QSqlQuery query(query_getDbStrategyData(strategy_id, m_uniqueConnectionName));
    if (!query.exec()) {
        qCDebug(lcDbHandler) << "Error fetching strategy data:" << query.lastError();
        emit signalStrategyDataFetched(obj, e_queryStatus::QS_ERROR);
    }
    else
    {
        if (query.next()) {  // Assuming only one result per strategy_id
            obj.strategyId = query.value("strategyId").toString();
            obj.availableBP = query.value("availableBP").toDouble();
            obj.usedBP = query.value("usedBP").toDouble();
            obj.realizedPnL = query.value("realizedPnL").toDouble();
            obj.unrealizedPnL = query.value("unrealizedPnL").toDouble();
            obj.pnlPercentage = query.value("pnlPercentage").toDouble();
            obj.fees = query.value("fees").toDouble();

            emit signalStrategyDataFetched(obj, e_queryStatus::QS_VALID);
        } else {
            qCDebug(lcDbHandler) << "No strategy data found for strategy_id:" << strategy_id;
            emit signalStrategyDataFetched(obj, e_queryStatus::QS_NOT_FOUND);
        }
    }
}

// ---------------------------------------------------------------------------
// Backtest write slots
// ---------------------------------------------------------------------------

void DBHandler::slotInsertBacktestRun(const DbBacktestRun& run) {
    auto q = query_insertBacktestRun(run, m_uniqueConnectionName);
    if (!q.exec())
        qCWarning(lcDbHandler) << "DBHandler: slotInsertBacktestRun failed:" << q.lastError().text();
}

void DBHandler::slotUpdateBacktestRunStatus(const QString& runId, const QString& status,
                                            const QString& errorText, qint64 durationMs,
                                            const QString& dataRefreshedAt) {
    auto q = query_updateBacktestRunStatus(runId, status, errorText, durationMs,
                                            dataRefreshedAt, m_uniqueConnectionName);
    if (!q.exec())
        qCWarning(lcDbHandler) << "DBHandler: slotUpdateBacktestRunStatus failed:" << q.lastError().text();
}

void DBHandler::slotInsertBacktestMetrics(const DbBacktestMetrics& metrics) {
    auto q = query_insertBacktestMetrics(metrics, m_uniqueConnectionName);
    if (!q.exec())
        qCWarning(lcDbHandler) << "DBHandler: slotInsertBacktestMetrics failed:" << q.lastError().text();
}

void DBHandler::slotInsertBacktestTrades(const QList<DbBacktestTrade>& trades) {
    if (trades.isEmpty()) return;
    m_db.transaction();
    for (const auto& t : trades) {
        auto q = query_insertBacktestTrade(t, m_uniqueConnectionName);
        if (!q.exec())
            qCWarning(lcDbHandler) << "DBHandler: slotInsertBacktestTrades failed:" << q.lastError().text();
    }
    m_db.commit();
}

void DBHandler::slotInsertEquityCurve(const QList<DbBacktestEquityPoint>& points) {
    if (points.isEmpty()) return;
    m_db.transaction();
    for (const auto& p : points) {
        auto q = query_insertEquityPoint(p, m_uniqueConnectionName);
        if (!q.exec())
            qCWarning(lcDbHandler) << "DBHandler: slotInsertEquityCurve failed:" << q.lastError().text();
    }
    m_db.commit();
}

void DBHandler::slotUpsertHistoricalBars(const QList<DbHistoricalBar>& bars) {
    if (bars.isEmpty()) return;
    m_db.transaction();
    for (const auto& b : bars) {
        auto q = query_upsertHistoricalBar(b, m_uniqueConnectionName);
        if (!q.exec())
            qCWarning(lcDbHandler) << "DBHandler: slotUpsertHistoricalBars failed:" << q.lastError().text();
    }
    m_db.commit();
}

// ---------------------------------------------------------------------------
// Backtest read slots
// ---------------------------------------------------------------------------

void DBHandler::slotFetchRunsForStrategy(const QString& strategyId) {
    QList<DbBacktestRunSummary> result;
    auto q = query_fetchRunsForStrategy(strategyId, m_uniqueConnectionName);
    if (!q.exec()) {
        qCWarning(lcDbHandler) << "DBHandler: slotFetchRunsForStrategy failed:" << q.lastError().text();
        emit signalRunsForStrategyFetched(result);
        return;
    }
    while (q.next()) {
        DbBacktestRunSummary s;
        s.runId        = q.value("runId").toString();
        s.strategyId   = q.value("strategyId").toString();
        s.symbols      = q.value("symbols").toString();
        s.startDate    = q.value("startDate").toString();
        s.endDate      = q.value("endDate").toString();
        s.status       = q.value("status").toString();
        s.dataSourceId = q.value("dataSourceId").toString();
        s.createdAt    = q.value("createdAt").toString();
        s.totalReturn  = q.value("totalReturn").toDouble();
        s.sharpeRatio  = q.value("sharpeRatio").toDouble();
        result.append(s);
    }
    emit signalRunsForStrategyFetched(result);
}

void DBHandler::slotFetchLoadedRun(const QString& runId) {
    DbBacktestRun run;
    DbBacktestMetrics metrics;
    QList<DbBacktestTrade> trades;
    QList<DbBacktestEquityPoint> equity;

    // Fetch run record
    {
        auto q = query_fetchBacktestRun(runId, m_uniqueConnectionName);
        if (q.exec() && q.next()) {
            run.runId               = q.value("runId").toString();
            run.strategyId          = q.value("strategyId").toString();
            run.strategyDisplayName = q.value("strategyDisplayName").toString();
            run.portfolioPath       = q.value("portfolioPath").toString();
            run.configJson          = q.value("configJson").toString();
            run.symbols             = q.value("symbols").toString();
            run.startDate           = q.value("startDate").toString();
            run.endDate             = q.value("endDate").toString();
            run.status              = q.value("status").toString();
            run.errorText           = q.value("errorText").toString();
            run.durationMs          = q.value("durationMs").toLongLong();
            run.engineVersion       = q.value("engineVersion").toString();
            run.dataSourceId        = q.value("dataSourceId").toString();
            run.dataRefreshedAt     = q.value("dataRefreshedAt").toString();
            run.createdAt           = q.value("createdAt").toString();
            run.strategyDefId       = q.value("strategyDefId").toString();
            run.scopeType           = q.value("scopeType").toString();
            run.scopeRefId          = q.value("scopeRefId").toString();
            run.strategyVersion     = q.value("strategyVersion").toInt();
            run.catalogStrategyId   = q.value("catalogStrategyId").toString();
            run.catalogVersionId    = q.value("catalogVersionId").toString();
        }
    }

    // Fetch metrics
    {
        auto q = query_fetchBacktestMetrics(runId, m_uniqueConnectionName);
        if (q.exec() && q.next()) {
            metrics.runId            = runId;
            metrics.totalReturn      = q.value("totalReturn").toDouble();
            metrics.annualizedReturn = q.value("annualizedReturn").toDouble();
            metrics.sharpeRatio      = q.value("sharpeRatio").toDouble();
            metrics.maxDrawdown      = q.value("maxDrawdown").toDouble();
            metrics.winRate          = q.value("winRate").toDouble();
            metrics.totalTrades      = q.value("totalTrades").toInt();
            metrics.initialCapital   = q.value("initialCapital").toDouble();
            metrics.finalCapital     = q.value("finalCapital").toDouble();
            metrics.benchmarkReturn  = q.value("benchmarkReturn").toDouble();
            metrics.benchmarkSharpe  = q.value("benchmarkSharpe").toDouble();
            metrics.alpha            = q.value("alpha").toDouble();
        }
    }

    // Fetch trades
    {
        auto q = query_fetchBacktestTrades(runId, m_uniqueConnectionName);
        if (q.exec()) {
            while (q.next()) {
                DbBacktestTrade t;
                t.runId     = runId;
                t.symbol    = q.value("symbol").toString();
                t.side      = q.value("side").toString();
                t.quantity  = q.value("quantity").toDouble();
                t.fillPrice = q.value("fillPrice").toDouble();
                t.timestamp = q.value("timestamp").toString();
                trades.append(t);
            }
        }
    }

    // Fetch equity curve
    {
        auto q = query_fetchEquityCurve(runId, m_uniqueConnectionName);
        if (q.exec()) {
            while (q.next()) {
                DbBacktestEquityPoint p;
                p.runId          = runId;
                p.timestamp      = q.value("timestamp").toString();
                p.value          = q.value("value").toDouble();
                p.benchmarkValue = q.value("benchmarkValue").toDouble();
                equity.append(p);
            }
        }
    }

    emit signalLoadedRunFetched(run, metrics, trades, equity);
}

void DBHandler::slotFetchHistoricalBars(const QString& symbol, const QString& resolution,
                                         const QString& dataSourceId,
                                         const QString& fromUtc, const QString& toUtc) {
    QList<DbHistoricalBar> bars;
    auto q = query_fetchHistoricalBars(symbol, resolution, dataSourceId,
                                        fromUtc, toUtc, m_uniqueConnectionName);
    if (!q.exec()) {
        qCWarning(lcDbHandler) << "DBHandler: slotFetchHistoricalBars failed:" << q.lastError().text();
        emit signalHistoricalBarsFetched(symbol, resolution, dataSourceId, bars);
        return;
    }
    while (q.next()) {
        DbHistoricalBar b;
        b.symbol       = symbol;
        b.resolution   = resolution;
        b.dataSourceId = dataSourceId;
        b.timestamp    = q.value("timestamp").toString();
        b.open         = q.value("open").toDouble();
        b.high         = q.value("high").toDouble();
        b.low          = q.value("low").toDouble();
        b.close        = q.value("close").toDouble();
        b.volume       = q.value("volume").toDouble();
        bars.append(b);
    }
    emit signalHistoricalBarsFetched(symbol, resolution, dataSourceId, bars);
}

void DBHandler::slotFetchCachedBarRange(const QString& symbol, const QString& resolution,
                                         const QString& dataSourceId) {
    auto q = query_cachedBarRange(symbol, resolution, dataSourceId, m_uniqueConnectionName);
    QString minTs, maxTs;
    if (q.exec() && q.next()) {
        minTs = q.value("minTs").toString();
        maxTs = q.value("maxTs").toString();
    }
    emit signalCachedBarRangeFetched(symbol, resolution, dataSourceId, minTs, maxTs);
}

// ---------------------------------------------------------------------------
// Backtest run profile slots
// ---------------------------------------------------------------------------

void DBHandler::slotInsertBacktestRunProfile(const DbBacktestRunProfile& profile) {
    auto q = query_insertBacktestRunProfile(profile, m_uniqueConnectionName);
    if (!q.exec())
        qCWarning(lcDbHandler) << "DBHandler: slotInsertBacktestRunProfile failed:" << q.lastError().text();
}

void DBHandler::slotFetchRunProfilesForOwner(const QString& ownerType, const QString& ownerRefId) {
    QList<DbBacktestRunProfile> result;
    auto q = query_fetchRunProfilesForOwner(ownerType, ownerRefId, m_uniqueConnectionName);
    if (!q.exec()) {
        qCWarning(lcDbHandler) << "DBHandler: slotFetchRunProfilesForOwner failed:" << q.lastError().text();
        emit signalRunProfilesFetched(result);
        return;
    }
    while (q.next()) {
        DbBacktestRunProfile p;
        p.profileId     = q.value("profile_id").toString();
        p.ownerType     = q.value("owner_type").toString();
        p.ownerRefId    = q.value("owner_ref_id").toString();
        p.name          = q.value("name").toString();
        p.runConfigJson = q.value("run_config_json").toString();
        p.createdAt     = q.value("created_at").toString();
        p.updatedAt     = q.value("updated_at").toString();
        result.append(p);
    }
    emit signalRunProfilesFetched(result);
}

void DBHandler::slotFetchRunsForDefinition(const QString& strategyDefId) {
    QList<DbBacktestRunSummary> result;
    auto q = query_fetchRunsForDefinition(strategyDefId, m_uniqueConnectionName);
    if (!q.exec()) {
        qCWarning(lcDbHandler) << "DBHandler: slotFetchRunsForDefinition failed:" << q.lastError().text();
        emit signalRunsForDefinitionFetched(result);
        return;
    }
    while (q.next()) {
        DbBacktestRunSummary s;
        s.runId           = q.value("runId").toString();
        s.strategyId      = q.value("strategyId").toString();
        s.symbols         = q.value("symbols").toString();
        s.startDate       = q.value("startDate").toString();
        s.endDate         = q.value("endDate").toString();
        s.status          = q.value("status").toString();
        s.dataSourceId    = q.value("dataSourceId").toString();
        s.createdAt       = q.value("createdAt").toString();
        s.strategyDefId   = q.value("strategyDefId").toString();
        s.scopeType       = q.value("scopeType").toString();
        s.scopeRefId      = q.value("scopeRefId").toString();
        s.strategyVersion    = q.value("strategyVersion").isNull() ? 1 : q.value("strategyVersion").toInt();
        s.catalogStrategyId  = q.value("catalogStrategyId").toString();
        s.catalogVersionId   = q.value("catalogVersionId").toString();
        s.totalReturn        = q.value("totalReturn").toDouble();
        s.sharpeRatio        = q.value("sharpeRatio").toDouble();
        result.append(s);
    }
    emit signalRunsForDefinitionFetched(result);
}


