#include "tst_historical_data_manager_cache.h"

#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/HistoricalDataManager.h"
#include "DB/dbquery.h" // CREATE_TABLE_HISTORICAL_BARS

#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryFile>
#include <QUuid>
#include <QTimeZone>

using namespace Backtest;

static void createHistoricalBarsTable(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery     q(db);
    QVERIFY(q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS)));
}

/// Insert one daily bar per calendar day at 21:00 UTC (Yahoo-style US equity close).
static void insertSpyDailyRange(const QString& connName, QDate first, QDate last)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    db.transaction();
    QSqlQuery q(db);
    q.prepare(
        QLatin1String("INSERT OR REPLACE INTO HistoricalBars "
                      "(symbol, resolution, dataSourceId, timestamp, open, high, low, close, volume) "
                      "VALUES ('SPY','Day1','yahoo',:ts,100,101,99,100.5,1e6)"));
    for (QDate d = first; d <= last; d = d.addDays(1)) {
        const QDateTime ts(d, QTime(21, 0), QTimeZone::utc());
        q.bindValue(QStringLiteral(":ts"), ts.toUTC().toString(Qt::ISODate));
        QVERIFY(q.exec());
    }
    QVERIFY(db.commit());
}

void TestHistoricalDataManagerCache::preloadedRange_endOfDayTo_doesNotTriggerYahooGet()
{
    const QString conn = QStringLiteral("hist_cache_tst_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        insertSpyDailyRange(conn, QDate(2020, 1, 1), QDate(2020, 1, 10));
    }

    MockNetworkAccessManager nam;
    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2020, 1, 10), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, &refreshed);

    QCOMPARE(nam.requestCount(), 0);
    QVERIFY(refreshed.isEmpty());
    QCOMPARE(bars.size(), 10);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::secondIdenticalGetBars_doesNotIncrementRequestCount()
{
    const QString conn = QStringLiteral("hist_cache_tst2_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        insertSpyDailyRange(conn, QDate(2020, 1, 1), QDate(2020, 1, 10));
    }

    MockNetworkAccessManager nam;
    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2020, 1, 10), QTime(23, 59, 59), QTimeZone::utc());

    QString r1;
    mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"), QStringLiteral("yahoo"), from, to, &r1);
    const int c1 = nam.requestCount();

    QString r2;
    mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"), QStringLiteral("yahoo"), from, to, &r2);
    const int c2 = nam.requestCount();

    QCOMPARE(c1, 0);
    QCOMPARE(c2, 0);
    QVERIFY(r1.isEmpty());
    QVERIFY(r2.isEmpty());

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}
