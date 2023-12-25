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
    emit signalExecuteDatabaseQuery(queryStr);
}

void DBManager::addCurrentPositionsState(const OpenPosition & position)
{
    QString queryStr = QString("INSERT INTO open_positions (strategyId, symbol, quantity, price, pnl, fee, date, status) "
                               "VALUES (%1, '%2', %3, %4, %5, %6, '%7', %8)")
                           .arg(position.strategyId)
                           .arg(position.symbol)
                           .arg(position.quantity)
                           .arg(position.price)
                           .arg(position.pnl)
                           .arg(position.fee)
                           .arg(position.date)
                           .arg(position.status);
    emit signalExecuteDatabaseQuery(queryStr);
}
