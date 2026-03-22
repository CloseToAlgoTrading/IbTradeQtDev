#include "db/tst_dbhandler_disconnect.h"
#include "dbhandler.h"

#include <QtSql/QSqlDatabase>
#include <QTemporaryFile>

void TestDbHandlerDisconnect::disconnect_clearsNamedConnection()
{
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();

    DBHandler h;
    const QString name = h.connectionName();
    QVERIFY(!name.isEmpty());

    QVERIFY(h.connectDB(path));
    QVERIFY(QSqlDatabase::contains(name));

    h.disconnectDB();
    QVERIFY(!QSqlDatabase::contains(name));
}
