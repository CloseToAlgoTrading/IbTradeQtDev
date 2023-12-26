#include "dbmanager.h"
#include "dbquery.h"
#include "qdebug.h"

DBManager::DBManager(QObject *parent)
    : QObject{parent}
{
    m_db = std::make_unique<DBHandler>();
    m_dbThread.reset(new QThread(this));

    m_db->moveToThread(m_dbThread.data());

    connect(this, &DBManager::signalExecuteDatabaseQuery, m_db.get(), &DBHandler::executeQuerySlot, Qt::QueuedConnection);
    connect(this, &DBManager::signalExecuteGetOpenPositionsQuery, m_db.get(), &DBHandler::fetchOpenPositionsSlot, Qt::QueuedConnection);
    connect(m_db.get(), &DBHandler::openPositionsFetched, this, &DBManager::onOpenPositionsFetched, Qt::QueuedConnection);
    connect(m_dbThread.data(), &QThread::started, m_db.get(), &DBHandler::initializeConnectionSlot, Qt::QueuedConnection);

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
    emit signalExecuteDatabaseQuery(query_addCurrentPosition(position));
}

void DBManager::getOpenPositions(const quint16 strategy_id)
{
    emit signalExecuteGetOpenPositionsQuery(strategy_id);
}

void DBManager::onOpenPositionsFetched(const QList<OpenPosition> &positions)
{
    for (auto pos : positions) {
        qDebug() << pos.symbol.toStdString().c_str() << pos.status << pos.date;
    }
}
