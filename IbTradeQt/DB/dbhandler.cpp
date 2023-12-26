#include "dbhandler.h"
#include "dbquery.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>

DBHandler::DBHandler(QObject *parent) : QObject(parent) {
    // Constructor code
}

DBHandler::~DBHandler() {
    disconnectDB();
}

bool DBHandler::connectDB(const QString& dbName) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qDebug() << "Error: Connection with database failed";
        return false;
    }

    return initializeDatabase();
}

void DBHandler::disconnectDB() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBHandler::initializeDatabase() {
    // Initialize your database tables here
    QSqlQuery query;
    bool success = true;
    // bool success = query.exec("CREATE TABLE IF NOT EXISTS trades ("
    //                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    //                           "strategyId INTEGER, "
    //                           "symbol VARCHAR(10), "
    //                           "quantity INTEGER, "
    //                           "price REAL, "
    //                           "date TEXT)");
    // if (!success) {
    //     qDebug() << "Failed to create table:" << query.lastError();
    // }
    if ( !m_db.tables().contains( QString("open_positions") ) )
    {
        success = query.exec("CREATE TABLE IF NOT EXISTS open_positions ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "strategyId INTEGER, "
                                  "symbol VARCHAR(10), "
                                  "quantity INTEGER, "
                                  "price REAL, "
                                  "pnl REAL, "
                                  "fee REAL, "
                                  "date TEXT, "
                                  "status INTEGER)");
        if (!success) {
            qDebug() << "Failed to create table:" << query.lastError();
        }
    }
    return success;
}

void DBHandler::executeQuerySlot(const QString& queryStr) {
    QSqlQuery query(queryStr);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        // Process query results if needed
    }
}

void DBHandler::initializeConnectionSlot()
{
    connectDB("myLocalDb.sqlite");
    initializeDatabase();
}

void DBHandler::fetchOpenPositionsSlot(const quint16 strategy_id)
{
    QList<OpenPosition> positionsList;
    QSqlQuery query(query_getOpenPositions());
    if (!query.exec()) {
        qDebug() << "Error fetching open positions:" << query.lastError();
    }
    else
    {
        while (query.next()) {
            OpenPosition position;
            position.id = query.value("id").toInt();
            position.strategyId = query.value("strategyId").toInt();
            position.symbol = query.value("symbol").toString();
            position.quantity = query.value("quantity").toInt();
            position.price = query.value("price").toDouble();
            position.pnl = query.value("pnl").toDouble();
            position.fee = query.value("fee").toDouble();
            position.date = query.value("date").toString();
            position.status = query.value("status").toInt();

            positionsList.append(position);
        }
    }
    emit openPositionsFetched(positionsList);
}
