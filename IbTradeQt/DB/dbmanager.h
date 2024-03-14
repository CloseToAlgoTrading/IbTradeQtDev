#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QThread>
#include "dbhandler.h"
#include <memory>
#include <QScopedPointer>
#include "dbdatatypes.h"



class DBManager : public QObject
{
    Q_OBJECT
public:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();
    void addCurrentPositionsState(const OpenPosition & position);
    void getOpenPositions(const QString &strategy_id);

signals:
    void signalAddPositionQuery(const OpenPosition& position);
    void signalExecuteGetOpenPositionsQuery(const QString& strategy_id);
    void signalAddNewTrade(const DbTrade& trade);
    void signalUpdateTradeCommision(const DbTradeCommission& trade);
    void signalOpenPositionsFetched(const QList<OpenPosition>& positions);

private slots:
    void onOpenPositionsFetched(const QList<OpenPosition>& positions);

private:
    std::unique_ptr<DBHandler> m_db;
    QScopedPointer<QThread> m_dbThread;

};

#endif // DBMANAGER_H
