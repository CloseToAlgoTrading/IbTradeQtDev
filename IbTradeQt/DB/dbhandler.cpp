#include "dbhandler.h"
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
    bool success = query.exec("CREATE TABLE IF NOT EXISTS trades ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "strategyId INTEGER, "
                              "symbol VARCHAR(10), "
                              "quantity INTEGER, "
                              "price REAL, "
                              "date TEXT)");
    if (!success) {
        qDebug() << "Failed to create table:" << query.lastError();
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
