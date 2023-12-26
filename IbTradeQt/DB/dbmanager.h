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

    void addTrades(const quint16 stragegyId, const QString& symbol, int quantity, double price, const QString& date);
    void addCurrentPositionsState(const OpenPosition & position);

private:
    std::unique_ptr<DBHandler> m_db;
    QScopedPointer<QThread> m_dbThread;

signals:
    void signalExecuteDatabaseQuery(const QString& queryStr);

};

#endif // DBMANAGER_H
