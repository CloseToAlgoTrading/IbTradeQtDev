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

signals:
    void openPositionsFetched(const QList<OpenPosition>& positions);

public slots:
    void slotAddPositionQuery(const OpenPosition& position);
    void initializeConnectionSlot();
    void fetchOpenPositionsSlot(const quint16 strategy_id);

private:
    QSqlDatabase m_db;
    bool initializeDatabase();



};

#endif // DBHANDLER_H
