#ifndef DBHANDLER_H
#define DBHANDLER_H

#include "dbdatatypes.h"
#include <QObject>
#include <QtSql/QSqlDatabase>

class DBHandler : public QObject {
    Q_OBJECT

public:
    explicit DBHandler(QObject *parent = nullptr);
    ~DBHandler();

    bool connectDB(const QString& dbName);
    void disconnectDB();

    bool createTrigger(QSqlDatabase& db);

private:
    QString m_uniqueConnectionName;
signals:
    void openPositionsFetched(const QList<OpenPosition>& positions);
    void signalStrategyInfoFetched(const DbStrategyInfo& obj, bool isValid);
    void signalStrategyDataFetched(const DbStrategyData& obj, bool isValid);

    void signalDBConnectionState(const bool state);

public slots:
    void slotAddPositionQuery(const OpenPosition& position);
    void slotAddNewTrade(const DbTrade& trade);
    void slotUpdateTradeCommission(const DbTradeCommission& tradeComm);
    void initializeConnectionSlot();
    void fetchOpenPositionsSlot(const QString& strategy_id);

    void slotAddOrUpdateDbStrategyInfo(const DbStrategyInfo& obj);
    void slotGetStrategyInfo(const QString& strategy_id);
    void slotAddOrUpdateDbStrategyData(const DbStrategyData& obj);
    void slotGetStrategyData(const QString& strategy_id);

private:
    QSqlDatabase m_db;
    bool initializeDatabase();



};

#endif // DBHANDLER_H
