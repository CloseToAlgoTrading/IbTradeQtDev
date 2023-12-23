#include "dbmanager.h"
#include "qsqlquery.h"

DBManager::DBManager(QObject *parent)
    : QObject{parent}
{
    m_db = std::make_unique<DBHandler>();
    m_dbThread.reset(new QThread(this));

    m_db->moveToThread(m_dbThread.data());

    connect(this, &DBManager::signalExecuteDatabaseQuery, m_db.get(), &DBHandler::executeQuerySlot);

    m_dbThread->start();
}

DBManager::~DBManager()
{
    m_dbThread->quit();
    m_dbThread->wait();
}

void DBManager::addTrades(const quint16 stragegyId, const QString& symbol, int quantity, double price, const QString& date)
{
    QString queryStr = QString("INSERT INTO trades (strategyId, symbol, quantity, price, date) VALUES ('%1', %2, %3, '%4', %4)")
                           .arg(stragegyId)
                           .arg(symbol)
                           .arg(quantity)
                           .arg(price)
                           .arg(date);
    // QSqlQuery query;
    // query.prepare("INSERT INTO trades (strategyId, symbol, quantity, price, date) VALUES (:strategyId, :symbol, :quantity, :price, :date)");
    // query.bindValue(":strategyId", stragegyId);
    // query.bindValue(":symbol", symbol);
    // query.bindValue(":quantity", quantity);
    // query.bindValue(":price", price);
    // query.bindValue(":date", date);
    emit signalExecuteDatabaseQuery(queryStr);
}
