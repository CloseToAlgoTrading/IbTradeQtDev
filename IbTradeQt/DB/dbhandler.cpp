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

    auto createTableIfNotExists = [this](const QString& tableName, const QString& creationQuery) {
        bool success = true;
        QSqlQuery query(m_db);

        if (!m_db.tables().contains(tableName)) {
            success = query.exec(creationQuery);
            if (!success) {
                qDebug() << "Failed to create table:" << query.lastError();
            }
        }

        return success;
    };



    // Initialize database tables
    bool success = createTableIfNotExists("Strategies",
                           "CREATE TABLE IF NOT EXISTS Strategies ("
                           "strategyId VARCHAR(64) PRIMARY KEY, "
                           "maxDrawDown DOUBLE, "
                           "pnlPerc DOUBLE, "
                           "pnl DOUBLE, "
                           "fee DOUBLE)"
                           );

    // Trades Table
    success |= createTableIfNotExists("Trades",
                           "CREATE TABLE IF NOT EXISTS Trades ("
                           "tradeId INT AUTO_INCREMENT PRIMARY KEY, "
                           "positionId INT, "
                           "symbol VARCHAR(10), "
                           "quantity INT, "
                           "price DOUBLE, "
                           "pnl DOUBLE, "
                           "fee DOUBLE, "
                           "date TEXT, "
                           "tradeType VARCHAR(10), "
                           "FOREIGN KEY (positionId) REFERENCES Positions(positionId))"
                           );

    // Positions Table
    success |= createTableIfNotExists("Positions",
                           "CREATE TABLE IF NOT EXISTS Positions ("
                           "strategyId VARCHAR(64), "
                           "positionId INT AUTO_INCREMENT PRIMARY KEY, "
                           "symbol VARCHAR(10), "
                           "quantity INT, "
                           "averageOpenPrice DOUBLE, "
                           "pnl DOUBLE, "
                           "fee DOUBLE, "
                           "openDate TEXT, "
                           "closeDate TEXT, "
                           "status INT, "
                           "FOREIGN KEY (strategyId) REFERENCES Strategies(strategyId))"
                           );

    success |= createTableIfNotExists("open_positions",
                                     "CREATE TABLE IF NOT EXISTS open_positions ("
                                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                           "strategyId INTEGER, "
                                           "symbol VARCHAR(10), "
                                           "quantity INTEGER, "
                                           "price REAL, "
                                           "pnl REAL, "
                                           "fee REAL, "
                                           "date TEXT, "
                                           "status INTEGER"
                                     ")"
                                     );

  return success;
}

void DBHandler::slotAddPositionQuery(const OpenPosition &position)
{
    auto query = query_addCurrentPosition(position);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError();
    } else {
        qDebug() << "Query executed successfully";  // Confirm successful execution
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
    QSqlQuery query(query_getOpenPositions(strategy_id));
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
