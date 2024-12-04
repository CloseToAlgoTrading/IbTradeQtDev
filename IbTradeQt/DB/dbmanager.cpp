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
    connect(this, &DBManager::signalGetOpenPositionsQuery, m_db.get(), &DBHandler::slotFetchOpenPositions, Qt::QueuedConnection);

    connect(this, &DBManager::signalAddOrUpdateDbStrategyData, m_db.get(), &DBHandler::slotAddOrUpdateDbStrategyData, Qt::QueuedConnection);
    connect(this, &DBManager::signalGetStrategyData, m_db.get(), &DBHandler::slotGetStrategyData, Qt::QueuedConnection);
    connect(this, &DBManager::signalAddOrUpdateDbModelInfo, m_db.get(), &DBHandler::slotAddOrUpdateDbModelInfo, Qt::QueuedConnection);
    connect(this, &DBManager::signalGetModelInfo, m_db.get(), &DBHandler::slotGetModelInfo, Qt::QueuedConnection);


    connect(m_db.get(), &DBHandler::signalOpenPositionsFetched, this, &DBManager::onOpenPositionsFetched, Qt::AutoConnection);
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
    emit signalGetOpenPositionsQuery(strategy_id);
}

void DBManager::onOpenPositionsFetched(const QList<OpenPosition> &positions,  e_queryStatus state)
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

DBHandler*DBManager::getDbHandler() const
{
    return m_db.get();
}
