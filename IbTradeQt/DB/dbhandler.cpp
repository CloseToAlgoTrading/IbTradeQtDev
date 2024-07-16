#include "dbhandler.h"
#include "dbquery.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QUuid>

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
        qDebug() << "Error: Connection with database failed";
        return false;
    }

    //return initializeDatabase();
    return true;
}

void DBHandler::disconnectDB() {
    if (m_db.isOpen()) {
        m_db.close();
    }

    QSqlDatabase::removeDatabase(m_uniqueConnectionName);
}

bool DBHandler::initializeDatabase() {

    auto createTableIfNotExists = [this](const QString& tableName, const QString& creationQuery) {
        bool success = true;
        QSqlQuery query(m_db);

        if (!m_db.tables().contains(tableName)) {
            success = query.exec(creationQuery);
            if (!success) {
                qDebug() << "Failed to create table" << tableName << ":" << query.lastError().text();
            }
        }

        return success;
    };



    // Initialize database tables
    // StrategyData Table
    bool success = createTableIfNotExists("StrategyData",
                                      "CREATE TABLE IF NOT EXISTS StrategyData ("
                                      "strategyId VARCHAR(64) PRIMARY KEY, "
                                      "availableBP DOUBLE, "
                                      "usedBP DOUBLE, "
                                      "realizedPnL DOUBLE, "
                                      "unrealizedPnL DOUBLE, "
                                      "pnlPercentage DOUBLE, "
                                      "fees DOUBLE)"
                                      );

    // StrategyInfo Table
    success |= createTableIfNotExists("StrategyInfo",
                                      "CREATE TABLE IF NOT EXISTS StrategyInfo ("
                                      "strategyId VARCHAR(64) PRIMARY KEY, "
                                      "strategyName VARCHAR(255), "
                                      "strategyDescription TEXT, "
                                      "createdAt DATETIME, "
                                      "updatedAt DATETIME, "
                                      "status VARCHAR(64), "
                                      "currency VARCHAR(64), "
                                      "initialBP DOUBLE)"
                                      );

    // Trades Table
    success |= createTableIfNotExists("Trades",
                           "CREATE TABLE IF NOT EXISTS Trades ("
                           "execId VARCHAR(50) PRIMARY KEY, "
                           "strategyId VARCHAR(64), "
                           "symbol VARCHAR(10), "
                           "quantity INT, "
                           "price DOUBLE, "
                           "pnl DOUBLE, "
                           "fee DOUBLE, "
                           "date TEXT, "
                           "tradeType VARCHAR(10), "
                           "FOREIGN KEY (strategyId) REFERENCES StrategyInfo(strategyId))"
                           );

    // Positions Table
    success |= createTableIfNotExists("Positions",
                           R"(CREATE TABLE IF NOT EXISTS Positions (
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
                                    FOREIGN KEY (strategyId) REFERENCES StrategyInfo(strategyId)
                                )
                            )"
                           );

    success |= createTrigger(m_db);

  return success;
}

bool DBHandler::createTrigger(QSqlDatabase& db) {
    QSqlQuery query(db);
    QString triggerCommand = R"(
        CREATE TRIGGER IF NOT EXISTS update_or_insert_position
        AFTER INSERT ON Trades
        FOR EACH ROW
        BEGIN
            -- Check if the trade is a "SELL" trade and adjust the new quantity accordingly
            INSERT INTO Positions (strategyId, symbol, quantity, averageOpenPrice, pnl, fee, openDate, status)
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
            DELETE FROM Positions WHERE strategyId = NEW.strategyId AND symbol = NEW.symbol AND quantity = 0;
        END;

    )";

    if (!query.exec(triggerCommand)) {
        qDebug() << "Error creating trigger:" << query.lastError().text();
        return false;
    }

    return true;
}


void DBHandler::slotAddPositionQuery(const OpenPosition &position)
{
    auto query = query_addCurrentPosition(position, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotAddNewTrade(const DbTrade &trade)
{
    auto query = query_addNewTrade(trade, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotUpdateTradeCommission(const DbTradeCommission &tradeComm)
{
    auto query = query_updateTrade(tradeComm, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }

}

void DBHandler::initializeConnectionSlot()
{
    bool isConnected = connectDB("myLocalDb.sqlite");
    if(isConnected)
        isConnected = initializeDatabase();
    emit signalDBConnectionState(isConnected);
}

void DBHandler::fetchOpenPositionsSlot(const QString& strategy_id)
{
    QList<OpenPosition> positionsList;
    QSqlQuery query(query_getOpenPositions(strategy_id, m_uniqueConnectionName));
    if (!query.exec()) {
        qDebug() << "Error fetching open positions:" << query.lastError();
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
    }
    emit openPositionsFetched(positionsList);
}

void DBHandler::slotAddOrUpdateDbStrategyInfo(const DbStrategyInfo &obj)
{
    auto query = query_addOrUpdateDbStrategyInfo(obj, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotGetStrategyInfo(const QString &strategy_id)
{
    DbStrategyInfo obj;
    QSqlQuery query(query_getDbStrategyInfo(strategy_id, m_uniqueConnectionName));
    if (!query.exec()) {
        qDebug() << "Error fetching strategy info:" << query.lastError();
        emit signalStrategyInfoFetched(obj, false);
    }
    else
    {
        if (query.next()) {  // Assuming only one result per strategy_id
            obj.strategyId = query.value("strategyId").toString();
            obj.strategyName = query.value("strategyName").toString();
            obj.strategyDescription = query.value("strategyDescription").toString();
            obj.createdAt = query.value("createdAt").toDateTime();
            obj.updatedAt = query.value("updatedAt").toDateTime();
            obj.status = query.value("status").toString();
            obj.currency = query.value("currency").toString();
            obj.initialBP = query.value("initialBP").toDouble();

            emit signalStrategyInfoFetched(obj, true);
        } else {
            qDebug() << "No strategy info found for strategy_id:" << strategy_id;
            emit signalStrategyInfoFetched(obj, false);
        }
    }
}


void DBHandler::slotAddOrUpdateDbStrategyData(const DbStrategyData &obj)
{
    auto query = query_addOrUpdateDbStrategyData(obj, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotGetStrategyData(const QString &strategy_id)
{
    DbStrategyData obj;
    QSqlQuery query(query_getDbStrategyData(strategy_id, m_uniqueConnectionName));
    if (!query.exec()) {
        qDebug() << "Error fetching strategy data:" << query.lastError();
        emit signalStrategyDataFetched(obj, false);
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

            emit signalStrategyDataFetched(obj, true);
        } else {
            qDebug() << "No strategy data found for strategy_id:" << strategy_id;
            emit signalStrategyDataFetched(obj, false);
        }
    }
}


