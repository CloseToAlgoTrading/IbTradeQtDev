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

    auto createTableIfNotExists = [this](const QString& tableName, const QString& creationQueryTemplate) {
        bool success = true;
        QSqlQuery query(m_db);

        if (!m_db.tables().contains(tableName)) {
            success = query.exec(creationQueryTemplate.arg(tableName));
            if (!success) {
                qDebug() << "Failed to create table" << tableName << ":" << query.lastError().text();
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

void DBHandler::slotAddOrUpdateDbModelInfo(const DbModelInfo &obj)
{
    auto query = query_addOrUpdateDbModelInfo(obj, m_uniqueConnectionName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
        // Process query results if needed
    }
}

void DBHandler::slotGetModelInfo(const QString &modelId)
{
    DbModelInfo obj;
    QSqlQuery query(query_getDbModelInfo(modelId, m_uniqueConnectionName));
    if (!query.exec()) {
        qDebug() << "Error fetching model info:" << query.lastError();
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
            qDebug() << "No strategy data found for strategy_id:" << strategy_id;
            emit signalStrategyDataFetched(obj, e_queryStatus::QS_NOT_FOUND);
        }
    }
}


