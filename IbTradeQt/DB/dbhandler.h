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

signals:
    void openPositionsFetched(const QList<OpenPosition>& positions);

public slots:
    void slotAddPositionQuery(const OpenPosition& position);
    void slotAddNewTrade(const DbTrade& trade);
    void slotUpdateTradeCommission(const DbTradeCommission& tradeComm);
    void initializeConnectionSlot();
    void fetchOpenPositionsSlot(const quint16 strategy_id);





private:
    QSqlDatabase m_db;
    bool initializeDatabase();



};

#endif // DBHANDLER_H
