#include "dbmanager.h"
#include "qdebug.h"

DBManager::DBManager(QObject *parent)
    : QObject{parent}
{
    m_db = std::make_unique<DBHandler>();
    m_dbThread.reset(new QThread(this));

    m_db->moveToThread(m_dbThread.data());

    connect(this, &DBManager::signalAddPositionQuery, m_db.get(), &DBHandler::slotAddPositionQuery, Qt::QueuedConnection);
    connect(this, &DBManager::signalAddNewTrade, m_db.get(), &DBHandler::slotAddNewTrade, Qt::AutoConnection);
    connect(this, &DBManager::signalUpdateTradeCommision, m_db.get(), &DBHandler::slotUpdateTradeCommission, Qt::AutoConnection);
    connect(this, &DBManager::signalExecuteGetOpenPositionsQuery, m_db.get(), &DBHandler::fetchOpenPositionsSlot, Qt::QueuedConnection);

    connect(this, &DBManager::signalAddOrUpdateDbStrategyData, m_db.get(), &DBHandler::slotAddOrUpdateDbStrategyData, Qt::QueuedConnection);
    connect(this, &DBManager::signalGetStrategyData, m_db.get(), &DBHandler::slotGetStrategyData, Qt::QueuedConnection);
    connect(this, &DBManager::signalAddOrUpdateDbStrategyInfo, m_db.get(), &DBHandler::slotAddOrUpdateDbStrategyInfo, Qt::QueuedConnection);
    connect(this, &DBManager::signalGetStrategyInfo, m_db.get(), &DBHandler::slotGetStrategyInfo, Qt::QueuedConnection);


    connect(m_db.get(), &DBHandler::openPositionsFetched, this, &DBManager::onOpenPositionsFetched, Qt::AutoConnection);
    connect(m_dbThread.data(), &QThread::started, m_db.get(), &DBHandler::initializeConnectionSlot, Qt::QueuedConnection);

    connect(m_db.get(), &DBHandler::signalDBConnectionState, this, &DBManager::slotDbConnectionState, Qt::AutoConnection);

    m_dbThread->start();
}

DBManager::~DBManager()
{
    m_dbThread->quit();
    m_dbThread->wait();
}

void DBManager::addCurrentPositionsState(const OpenPosition & position)
{
    emit signalAddPositionQuery(position);
}

void DBManager::getOpenPositions(const QString& strategy_id)
{
    emit signalExecuteGetOpenPositionsQuery(strategy_id);
}

void DBManager::onOpenPositionsFetched(const QList<OpenPosition> &positions)
{
    for (auto const &pos : positions) {
        qDebug() << pos.symbol.toStdString().c_str() << pos.status << pos.date;
    }
    emit signalOpenPositionsFetched(positions);

}

void DBManager::slotDbConnectionState(const bool state)
{
    emit signalDBManagerState(state);
}
