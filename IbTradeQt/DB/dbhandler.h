#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QObject>
#include <QtSql/QSqlDatabase>

class DBHandler : public QObject {
    Q_OBJECT

public:
    explicit DBHandler(QObject *parent = nullptr);
    ~DBHandler();

    bool connectDB(const QString& dbName);
    void disconnectDB();

public slots:
    void executeQuerySlot(const QString &queryStr);

private:
    QSqlDatabase m_db;
    bool initializeDatabase();
};

#endif // DBHANDLER_H
