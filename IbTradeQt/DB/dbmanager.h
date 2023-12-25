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
    private:
    std::unique_ptr<DBHandler> m_db;
    QScopedPointer<QThread> m_dbThread;

    void addTrades(const quint16 stragegyId, const QString& symbol, int quantity, double price, const QString& date);
    void addCurrentPositionsState(const OpenPosition & position);
signals:
    void signalExecuteDatabaseQuery(const QString& queryStr);
signals:
};

#endif // DBMANAGER_H
