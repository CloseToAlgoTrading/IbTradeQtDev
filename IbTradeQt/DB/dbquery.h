#ifndef DBQUERY_H
#define DBQUERY_H

#include "dbdatatypes.h"
#include <QSqlQuery>


/* Create Tables */
const char* TABLE_STRATEGYDATA = "StrategyData";
const char* CREATE_TABLE_STRATEGYDATA_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %1 ("
    "strategyId VARCHAR(64) PRIMARY KEY, "
    "availableBP DOUBLE, "
    "usedBP DOUBLE, "
    "realizedPnL DOUBLE, "
    "unrealizedPnL DOUBLE, "
    "pnlPercentage DOUBLE, "
    "fees DOUBLE)";

const char* TABLE_MODELINFO = "ModelInfo";
const char* CREATE_TABLE_MODELINFO_TEMPLATE =
    "CREATE TABLE IF NOT EXISTS %1 ("
    "modelId VARCHAR(64) PRIMARY KEY, "
    "modelName VARCHAR(255), "
    "modelDescription TEXT, "
    "createdAt DATETIME, "
    "updatedAt DATETIME, "
    "status VARCHAR(64)) ";

const char* TABLE_TRADES = "Trades";
const char* CREATE_TABLE_TRADES_TEMPLATE =
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

const char* TABLE_POSITIONS = "Positions";
const char* CREATE_TABLE_POSITIONS_TEMPLATE =
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
const char* CREATE_UPDATE_OR_INSERT_TRIGGER_TEMPLATE = R"(
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


#endif // DBQUERY_H
