#ifndef DBQUERY_H
#define DBQUERY_H

#include "dbdatatypes.h"
#include <QSqlQuery>


/* Create Tables */
static const char* const TABLE_STRATEGYDATA = "StrategyData";
static const char* const CREATE_TABLE_STRATEGYDATA_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %1 ("
    "strategyId VARCHAR(64) PRIMARY KEY, "
    "availableBP DOUBLE, "
    "usedBP DOUBLE, "
    "realizedPnL DOUBLE, "
    "unrealizedPnL DOUBLE, "
    "pnlPercentage DOUBLE, "
    "fees DOUBLE)";

static const char* const TABLE_MODELINFO = "ModelInfo";
static const char* const CREATE_TABLE_MODELINFO_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %1 ("
    "modelId VARCHAR(64) PRIMARY KEY, "
    "modelName VARCHAR(255), "
    "modelDescription TEXT, "
    "createdAt DATETIME, "
    "updatedAt DATETIME, "
    "status VARCHAR(64)) ";

static const char* const TABLE_TRADES = "Trades";
static const char* const CREATE_TABLE_TRADES_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %1 ("
    "execId VARCHAR(50) PRIMARY KEY, "
    "strategyId VARCHAR(64), "
    "symbol VARCHAR(10), "
    "quantity INT, "
    "price DOUBLE, "
    "pnl DOUBLE, "
    "fee DOUBLE, "
    "date TEXT, "
    "tradeType VARCHAR(10), "
    "FOREIGN KEY (strategyId) REFERENCES ModelInfo(strategyId))";

static const char* const TABLE_POSITIONS = "Positions";
static const char* const CREATE_TABLE_POSITIONS_TEMPLATE =
    R"(CREATE TABLE IF NOT EXISTS %1 (
    strategyId VARCHAR(64),
    symbol VARCHAR(10),
    quantity INT DEFAULT 0,
    averageOpenPrice DOUBLE DEFAULT 0,
    pnl DOUBLE DEFAULT 0,
    fee DOUBLE DEFAULT 0,
    openDate TEXT,
    closeDate TEXT,
    status INT,
    PRIMARY KEY (strategyId, symbol),
    FOREIGN KEY (strategyId) REFERENCES ModelInfo(strategyId)
    ))";


/* Triggers */
static const char* const CREATE_UPDATE_OR_INSERT_TRIGGER_TEMPLATE = R"(
        CREATE TRIGGER IF NOT EXISTS update_or_insert_position
        AFTER INSERT ON %1
        FOR EACH ROW
        BEGIN
            -- Check if the trade is a "SELL" trade and adjust the new quantity accordingly
            INSERT INTO %2 (strategyId, symbol, quantity, averageOpenPrice, pnl, fee, openDate, status)
            VALUES (NEW.strategyId, NEW.symbol,
                    CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END,
                    NEW.price, NEW.pnl, NEW.fee, NEW.date, 1)
            ON CONFLICT (strategyId, symbol)
            DO
            UPDATE SET quantity = quantity +
                    CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END,
                   pnl = pnl + NEW.pnl,
                   fee = fee + NEW.fee,
                   averageOpenPrice = (averageOpenPrice * quantity + NEW.price *
                    CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END) /
                    (quantity + CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END),
                   openDate = CASE WHEN openDate IS NULL THEN NEW.date ELSE openDate END,
                   status = CASE WHEN quantity +
                    CASE WHEN NEW.tradeType = 'SELL' THEN -NEW.quantity ELSE NEW.quantity END = 0 THEN 0 ELSE status END;

            -- Delete the entry from the Positions table when quantity becomes zero
            DELETE FROM %2 WHERE strategyId = NEW.strategyId AND symbol = NEW.symbol AND quantity = 0;
        END;
    )";


/* Queries */

inline QSqlQuery query_addCurrentPosition(const OpenPosition &position, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("INSERT INTO open_positions (strategyId, symbol, quantity, price, pnl, fee, date, status) "
                  "VALUES (:strategyId, :symbol, :quantity, :price, :pnl, :fee, :date, :status)");

    query.bindValue(":strategyId", position.strategyId);
    query.bindValue(":symbol", position.symbol);
    query.bindValue(":quantity", position.quantity);
    query.bindValue(":price", position.price);
    query.bindValue(":pnl", position.pnl);
    query.bindValue(":fee", position.fee);
    query.bindValue(":date", position.date);
    query.bindValue(":status", position.status);

    return query;
}


inline QSqlQuery query_getOpenPositions(const QString& strategyId, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("SELECT * FROM positions WHERE strategyId = :strategyId");

    query.bindValue(":strategyId", strategyId);

    return query;
}


inline QSqlQuery  query_addNewTrade(const DbTrade& trade, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("INSERT INTO Trades (strategyId, execId, symbol, quantity, price, pnl, fee, date, tradeType) "
                  "VALUES (:strategyId, :execId, :symbol, :quantity, :price, :pnl, :fee, :date, :tradeType)");

    query.bindValue(":strategyId", trade.strategyId);
    query.bindValue(":execId", trade.execId);
    query.bindValue(":symbol", trade.symbol);
    query.bindValue(":quantity", trade.quantity);
    query.bindValue(":price", trade.price);
    query.bindValue(":pnl", trade.pnl);
    query.bindValue(":fee", trade.fee);
    query.bindValue(":date", trade.date);
    query.bindValue(":tradeType", trade.tradeType);

    return query;
}

inline QSqlQuery query_updateTrade(const DbTradeCommission& obj, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("UPDATE Trades "
                  "SET fee = :fee, "
                  "    pnl = :pnl "
                  "WHERE execId = :execId");

    query.bindValue(":fee", obj.fee);
    query.bindValue(":pnl", obj.pnl);
    query.bindValue(":execId", obj.execId);

    return query;
}

inline QSqlQuery query_addOrUpdateDbStrategyData(const DbStrategyData& obj, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("INSERT INTO StrategyData (strategyId, availableBP, usedBP, realizedPnL, unrealizedPnL, pnlPercentage, fees) "
                  "VALUES (:strategyId, :availableBP, :usedBP, :realizedPnL, :unrealizedPnL, :pnlPercentage, :fees) "
                  "ON CONFLICT(strategyId) DO UPDATE SET "
                  "availableBP = excluded.availableBP, "
                  "usedBP = excluded.usedBP, "
                  "realizedPnL = excluded.realizedPnL, "
                  "unrealizedPnL = excluded.unrealizedPnL, "
                  "pnlPercentage = excluded.pnlPercentage, "
                  "fees = excluded.fees");

    query.bindValue(":strategyId", obj.strategyId);
    query.bindValue(":availableBP", obj.availableBP);
    query.bindValue(":usedBP", obj.usedBP);
    query.bindValue(":realizedPnL", obj.realizedPnL);
    query.bindValue(":unrealizedPnL", obj.unrealizedPnL);
    query.bindValue(":pnlPercentage", obj.pnlPercentage);
    query.bindValue(":fees", obj.fees);

    return query;
}

inline QSqlQuery query_getDbStrategyData(const QString& strategyId, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("SELECT strategyId, availableBP, usedBP, realizedPnL, unrealizedPnL, pnlPercentage, fees "
                  "FROM StrategyData "
                  "WHERE strategyId = :strategyId");
    query.bindValue(":strategyId", strategyId);

    return query;
}

inline QSqlQuery query_addOrUpdateDbModelInfo(const DbModelInfo& obj, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("INSERT INTO ModelInfo (modelId, modelName, modelDescription, createdAt, updatedAt, status) "
                  "VALUES (:modelId, :modelName, :modelDescription, :createdAt, :updatedAt, :status) "
                  "ON CONFLICT(modelId) DO UPDATE SET "
                  "modelName = excluded.modelName, "
                  "modelDescription = excluded.modelDescription, "
                  "createdAt = excluded.createdAt, "
                  "updatedAt = excluded.updatedAt, "
                  "status = excluded.status ");

    query.bindValue(":modelId", obj.modelId);
    query.bindValue(":modelName", obj.modelName);
    query.bindValue(":modelDescription", obj.modelDescription);
    query.bindValue(":createdAt", obj.createdAt);
    query.bindValue(":updatedAt", obj.updatedAt);
    query.bindValue(":status", obj.status);

    return query;
}

inline QSqlQuery query_getDbModelInfo(const QString& modelId, const QString& uniqueConnectionName) {
    QSqlQuery query(QSqlDatabase::database(uniqueConnectionName));
    query.prepare("SELECT modelId, modelName, modelDescription, createdAt, updatedAt, status "
                  "FROM ModelInfo "
                  "WHERE modelId = :modelId");
    query.bindValue(":modelId", modelId);

    return query;
}

// ---------------------------------------------------------------------------
// Backtest table DDL
// ---------------------------------------------------------------------------

static const char* const TABLE_BACKTEST_RUNS = "BacktestRuns";
static const char* const CREATE_TABLE_BACKTEST_RUNS =
    "CREATE TABLE IF NOT EXISTS BacktestRuns ("
    "runId               TEXT PRIMARY KEY, "
    "strategyId          TEXT NOT NULL, "
    "strategyDisplayName TEXT, "
    "portfolioPath       TEXT, "
    "configJson          TEXT, "
    "symbols             TEXT, "
    "startDate           TEXT, "
    "endDate             TEXT, "
    "status              TEXT, "   // Created | Running | Finished | Failed
    "errorText           TEXT, "
    "durationMs          INTEGER DEFAULT 0, "
    "engineVersion       TEXT, "
    "dataSourceId        TEXT, "
    "dataRefreshedAt     TEXT, "
    "createdAt           TEXT)";

static const char* const TABLE_BACKTEST_METRICS = "BacktestMetrics";
static const char* const CREATE_TABLE_BACKTEST_METRICS =
    "CREATE TABLE IF NOT EXISTS BacktestMetrics ("
    "runId             TEXT PRIMARY KEY, "
    "totalReturn       REAL, "
    "annualizedReturn  REAL, "
    "sharpeRatio       REAL, "
    "maxDrawdown       REAL, "
    "winRate           REAL, "
    "totalTrades       INTEGER, "
    "initialCapital    REAL, "
    "finalCapital      REAL, "
    "benchmarkReturn   REAL, "
    "benchmarkSharpe   REAL, "
    "alpha             REAL)";

static const char* const TABLE_BACKTEST_TRADES = "BacktestTrades";
static const char* const CREATE_TABLE_BACKTEST_TRADES =
    "CREATE TABLE IF NOT EXISTS BacktestTrades ("
    "id         INTEGER PRIMARY KEY AUTOINCREMENT, "
    "runId      TEXT, "
    "symbol     TEXT, "
    "side       TEXT, "
    "quantity   REAL, "
    "fillPrice  REAL, "
    "timestamp  TEXT)";

static const char* const TABLE_BACKTEST_EQUITY_CURVE = "BacktestEquityCurve";
static const char* const CREATE_TABLE_BACKTEST_EQUITY_CURVE =
    "CREATE TABLE IF NOT EXISTS BacktestEquityCurve ("
    "id             INTEGER PRIMARY KEY AUTOINCREMENT, "
    "runId          TEXT, "
    "timestamp      TEXT, "
    "value          REAL, "
    "benchmarkValue REAL)";

// Historical bar cache.
// INSERT OR REPLACE: new fetch always trusted; no versioning.
// dataSourceId is part of PK so bars from yahoo and csv never mix.
// All timestamps stored as UTC ISO 8601.
static const char* const TABLE_HISTORICAL_BARS = "HistoricalBars";
static const char* const CREATE_TABLE_HISTORICAL_BARS =
    "CREATE TABLE IF NOT EXISTS HistoricalBars ("
    "symbol       TEXT NOT NULL, "
    "resolution   TEXT NOT NULL, "
    "dataSourceId TEXT NOT NULL, "
    "timestamp    TEXT NOT NULL, "
    "open         REAL, "
    "high         REAL, "
    "low          REAL, "
    "close        REAL, "
    "volume       REAL, "
    "PRIMARY KEY (symbol, resolution, dataSourceId, timestamp))";

// ---------------------------------------------------------------------------
// Backtest query functions
// ---------------------------------------------------------------------------

inline QSqlQuery query_insertBacktestRun(const DbBacktestRun& r, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT INTO BacktestRuns "
        "(runId, strategyId, strategyDisplayName, portfolioPath, configJson, symbols, "
        " startDate, endDate, status, errorText, durationMs, engineVersion, "
        " dataSourceId, dataRefreshedAt, createdAt, "
        " strategyDefId, scopeType, scopeRefId, strategyVersion) "
        "VALUES (:runId,:strategyId,:strategyDisplayName,:portfolioPath,:configJson,"
        ":symbols,:startDate,:endDate,:status,:errorText,:durationMs,:engineVersion,"
        ":dataSourceId,:dataRefreshedAt,:createdAt,"
        ":strategyDefId,:scopeType,:scopeRefId,:strategyVersion)");
    q.bindValue(":runId",               r.runId);
    q.bindValue(":strategyId",          r.strategyId);
    q.bindValue(":strategyDisplayName", r.strategyDisplayName);
    q.bindValue(":portfolioPath",        r.portfolioPath);
    q.bindValue(":configJson",           r.configJson);
    q.bindValue(":symbols",              r.symbols);
    q.bindValue(":startDate",            r.startDate);
    q.bindValue(":endDate",              r.endDate);
    q.bindValue(":status",               r.status);
    q.bindValue(":errorText",            r.errorText);
    q.bindValue(":durationMs",           r.durationMs);
    q.bindValue(":engineVersion",        r.engineVersion);
    q.bindValue(":dataSourceId",         r.dataSourceId);
    q.bindValue(":dataRefreshedAt",      r.dataRefreshedAt);
    q.bindValue(":createdAt",            r.createdAt);
    q.bindValue(":strategyDefId",        r.strategyDefId);
    q.bindValue(":scopeType",            r.scopeType);
    q.bindValue(":scopeRefId",           r.scopeRefId);
    q.bindValue(":strategyVersion",      r.strategyVersion);
    return q;
}

inline QSqlQuery query_updateBacktestRunStatus(const QString& runId,
                                               const QString& status,
                                               const QString& errorText,
                                               qint64 durationMs,
                                               const QString& dataRefreshedAt,
                                               const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "UPDATE BacktestRuns "
        "SET status = :status, errorText = :errorText, "
        "    durationMs = :durationMs, dataRefreshedAt = :dataRefreshedAt "
        "WHERE runId = :runId");
    q.bindValue(":runId",           runId);
    q.bindValue(":status",          status);
    q.bindValue(":errorText",       errorText);
    q.bindValue(":durationMs",      durationMs);
    q.bindValue(":dataRefreshedAt", dataRefreshedAt);
    return q;
}

inline QSqlQuery query_insertBacktestMetrics(const DbBacktestMetrics& m, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT OR REPLACE INTO BacktestMetrics "
        "(runId, totalReturn, annualizedReturn, sharpeRatio, maxDrawdown, winRate, "
        " totalTrades, initialCapital, finalCapital, benchmarkReturn, benchmarkSharpe, alpha) "
        "VALUES (:runId,:totalReturn,:annualizedReturn,:sharpeRatio,:maxDrawdown,:winRate,"
        ":totalTrades,:initialCapital,:finalCapital,:benchmarkReturn,:benchmarkSharpe,:alpha)");
    q.bindValue(":runId",            m.runId);
    q.bindValue(":totalReturn",      m.totalReturn);
    q.bindValue(":annualizedReturn", m.annualizedReturn);
    q.bindValue(":sharpeRatio",      m.sharpeRatio);
    q.bindValue(":maxDrawdown",      m.maxDrawdown);
    q.bindValue(":winRate",          m.winRate);
    q.bindValue(":totalTrades",      m.totalTrades);
    q.bindValue(":initialCapital",   m.initialCapital);
    q.bindValue(":finalCapital",     m.finalCapital);
    q.bindValue(":benchmarkReturn",  m.benchmarkReturn);
    q.bindValue(":benchmarkSharpe",  m.benchmarkSharpe);
    q.bindValue(":alpha",            m.alpha);
    return q;
}

inline QSqlQuery query_insertBacktestTrade(const DbBacktestTrade& t, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT INTO BacktestTrades (runId, symbol, side, quantity, fillPrice, timestamp) "
        "VALUES (:runId,:symbol,:side,:quantity,:fillPrice,:timestamp)");
    q.bindValue(":runId",     t.runId);
    q.bindValue(":symbol",    t.symbol);
    q.bindValue(":side",      t.side);
    q.bindValue(":quantity",  t.quantity);
    q.bindValue(":fillPrice", t.fillPrice);
    q.bindValue(":timestamp", t.timestamp);
    return q;
}

inline QSqlQuery query_insertEquityPoint(const DbBacktestEquityPoint& p, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT INTO BacktestEquityCurve (runId, timestamp, value, benchmarkValue) "
        "VALUES (:runId,:timestamp,:value,:benchmarkValue)");
    q.bindValue(":runId",          p.runId);
    q.bindValue(":timestamp",      p.timestamp);
    q.bindValue(":value",          p.value);
    q.bindValue(":benchmarkValue", p.benchmarkValue);
    return q;
}

inline QSqlQuery query_upsertHistoricalBar(const DbHistoricalBar& b, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT OR REPLACE INTO HistoricalBars "
        "(symbol, resolution, dataSourceId, timestamp, open, high, low, close, volume) "
        "VALUES (:symbol,:resolution,:dataSourceId,:timestamp,:open,:high,:low,:close,:volume)");
    q.bindValue(":symbol",       b.symbol);
    q.bindValue(":resolution",   b.resolution);
    q.bindValue(":dataSourceId", b.dataSourceId);
    q.bindValue(":timestamp",    b.timestamp);
    q.bindValue(":open",         b.open);
    q.bindValue(":high",         b.high);
    q.bindValue(":low",          b.low);
    q.bindValue(":close",        b.close);
    q.bindValue(":volume",       b.volume);
    return q;
}

// Returns runs for a strategy, joining with metrics for display.
// Ordered newest first.
inline QSqlQuery query_fetchRunsForStrategy(const QString& strategyId, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "SELECT r.runId, r.strategyId, r.symbols, r.startDate, r.endDate, "
        "       r.status, r.dataSourceId, r.createdAt, "
        "       COALESCE(m.totalReturn, 0) AS totalReturn, "
        "       COALESCE(m.sharpeRatio, 0) AS sharpeRatio "
        "FROM BacktestRuns r "
        "LEFT JOIN BacktestMetrics m ON r.runId = m.runId "
        "WHERE r.strategyId = :strategyId "
        "ORDER BY r.createdAt DESC");
    q.bindValue(":strategyId", strategyId);
    return q;
}

inline QSqlQuery query_fetchBacktestRun(const QString& runId, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT * FROM BacktestRuns WHERE runId = :runId");
    q.bindValue(":runId", runId);
    return q;
}

inline QSqlQuery query_fetchBacktestMetrics(const QString& runId, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT * FROM BacktestMetrics WHERE runId = :runId");
    q.bindValue(":runId", runId);
    return q;
}

inline QSqlQuery query_fetchBacktestTrades(const QString& runId, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT symbol, side, quantity, fillPrice, timestamp "
              "FROM BacktestTrades WHERE runId = :runId ORDER BY timestamp ASC");
    q.bindValue(":runId", runId);
    return q;
}

inline QSqlQuery query_fetchEquityCurve(const QString& runId, const QString& conn) {
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT timestamp, value, benchmarkValue "
              "FROM BacktestEquityCurve WHERE runId = :runId ORDER BY timestamp ASC");
    q.bindValue(":runId", runId);
    return q;
}

// Fetches cached bars for a symbol/resolution/source within a date range.
// Returns rows ordered by timestamp ASC.
inline QSqlQuery query_fetchHistoricalBars(const QString& symbol,
                                            const QString& resolution,
                                            const QString& dataSourceId,
                                            const QString& fromUtc,
                                            const QString& toUtc,
                                            const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "SELECT timestamp, open, high, low, close, volume "
        "FROM HistoricalBars "
        "WHERE symbol = :symbol AND resolution = :resolution AND dataSourceId = :dataSourceId "
        "  AND timestamp >= :from AND timestamp <= :to "
        "ORDER BY timestamp ASC");
    q.bindValue(":symbol",       symbol);
    q.bindValue(":resolution",   resolution);
    q.bindValue(":dataSourceId", dataSourceId);
    q.bindValue(":from",         fromUtc);
    q.bindValue(":to",           toUtc);
    return q;
}

// Returns the min and max cached timestamp for a (symbol, resolution, dataSourceId) triple.
inline QSqlQuery query_cachedBarRange(const QString& symbol,
                                       const QString& resolution,
                                       const QString& dataSourceId,
                                       const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "SELECT MIN(timestamp) AS minTs, MAX(timestamp) AS maxTs "
        "FROM HistoricalBars "
        "WHERE symbol = :symbol AND resolution = :resolution AND dataSourceId = :dataSourceId");
    q.bindValue(":symbol",       symbol);
    q.bindValue(":resolution",   resolution);
    q.bindValue(":dataSourceId", dataSourceId);
    return q;
}

// ---------------------------------------------------------------------------
// BacktestRunProfiles DDL
// owner_type: "strategy_definition" | "live_strategy" | "portfolio" | "account"
// ---------------------------------------------------------------------------

static const char* const TABLE_BACKTEST_RUN_PROFILES = "backtest_run_profiles";
static const char* const CREATE_TABLE_BACKTEST_RUN_PROFILES =
    "CREATE TABLE IF NOT EXISTS backtest_run_profiles ("
    "profile_id      TEXT PRIMARY KEY, "
    "owner_type      TEXT NOT NULL, "
    "owner_ref_id    TEXT NOT NULL, "
    "name            TEXT NOT NULL DEFAULT '', "
    "run_config_json TEXT NOT NULL DEFAULT '{}', "
    "created_at      TEXT NOT NULL, "
    "updated_at      TEXT NOT NULL)";

// ALTER TABLE migrations to extend BacktestRuns — errors silently ignored on re-run
static const char* const ALTER_BACKTEST_RUNS_ADD_STRATEGY_DEF_ID =
    "ALTER TABLE BacktestRuns ADD COLUMN strategyDefId   TEXT    DEFAULT ''";
static const char* const ALTER_BACKTEST_RUNS_ADD_SCOPE_TYPE =
    "ALTER TABLE BacktestRuns ADD COLUMN scopeType       TEXT    DEFAULT 'strategy'";
static const char* const ALTER_BACKTEST_RUNS_ADD_SCOPE_REF_ID =
    "ALTER TABLE BacktestRuns ADD COLUMN scopeRefId      TEXT    DEFAULT ''";
static const char* const ALTER_BACKTEST_RUNS_ADD_STRATEGY_VERSION =
    "ALTER TABLE BacktestRuns ADD COLUMN strategyVersion INTEGER DEFAULT 1";

// ---------------------------------------------------------------------------
// BacktestRunProfiles query functions
// ---------------------------------------------------------------------------

inline QSqlQuery query_insertBacktestRunProfile(const DbBacktestRunProfile& p, const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "INSERT INTO backtest_run_profiles "
        "(profile_id, owner_type, owner_ref_id, name, run_config_json, created_at, updated_at) "
        "VALUES (:pid, :otype, :oref, :name, :cfg, :created, :updated)");
    q.bindValue(":pid",     p.profileId);
    q.bindValue(":otype",   p.ownerType);
    q.bindValue(":oref",    p.ownerRefId);
    q.bindValue(":name",    p.name);
    q.bindValue(":cfg",     p.runConfigJson);
    q.bindValue(":created", p.createdAt);
    q.bindValue(":updated", p.updatedAt);
    return q;
}

inline QSqlQuery query_fetchRunProfilesForOwner(const QString& ownerType,
                                                  const QString& ownerRefId,
                                                  const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "SELECT * FROM backtest_run_profiles "
        "WHERE owner_type = :otype AND owner_ref_id = :oref "
        "ORDER BY updated_at DESC");
    q.bindValue(":otype", ownerType);
    q.bindValue(":oref",  ownerRefId);
    return q;
}

// Returns runs for a canonical strategy definition, joining with metrics for display.
// Ordered newest first.
inline QSqlQuery query_fetchRunsForDefinition(const QString& strategyDefId, const QString& conn)
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare(
        "SELECT r.runId, r.strategyId, r.strategyDefId, r.scopeType, r.scopeRefId, "
        "       r.symbols, r.startDate, r.endDate, "
        "       r.status, r.dataSourceId, r.createdAt, r.strategyVersion, "
        "       COALESCE(m.totalReturn, 0) AS totalReturn, "
        "       COALESCE(m.sharpeRatio, 0) AS sharpeRatio "
        "FROM BacktestRuns r "
        "LEFT JOIN BacktestMetrics m ON r.runId = m.runId "
        "WHERE r.strategyDefId = :defId "
        "ORDER BY r.createdAt DESC");
    q.bindValue(":defId", strategyDefId);
    return q;
}

#endif // DBQUERY_H
