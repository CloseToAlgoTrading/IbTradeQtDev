#ifndef TST_DBHANDLER_DISCONNECT_H
#define TST_DBHANDLER_DISCONNECT_H

#include <QObject>
#include <QtTest>

class TestDbHandlerDisconnect : public QObject {
    Q_OBJECT
private slots:
    void disconnect_clearsNamedConnection();
};

#endif // TST_DBHANDLER_DISCONNECT_H
