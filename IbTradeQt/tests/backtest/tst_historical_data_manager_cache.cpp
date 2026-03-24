#include "tst_historical_data_manager_cache.h"

#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/HistoricalDataManager.h"
#include "DB/dbquery.h" // CREATE_TABLE_HISTORICAL_BARS
#include "Strategies/Generic/mandatoryFieldKeys.h"

#include <QtTest>
#include <QHash>
#include <QVariantMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryFile>
#include <QUuid>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>

using namespace Backtest;

static void createHistoricalBarsTable(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery     q(db);
    QVERIFY(q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS)));
}

namespace {

QByteArray twoDayMockYahooChartJson(const QString& symbol)
{
    const qint64 base =
        QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
        {base + 86400, 100.5, 102.0, 100.0, 101.25, 1.1e6},
    };
    return buildYahooJson(symbol, rows);
}

} // namespace

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

void TestHistoricalDataManagerCache::getBarsMulti_trailingGap_yahooPeriodSpansMissingTail()
{
    const QString conn =
        QStringLiteral("hist_cache_trail_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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
    // Mock timestamps must fall inside the requested backtest window (2020); twoDayMockYahooChartJson uses 2024.
    const qint64 jan15_2020_close =
        QDateTime(QDate(2020, 1, 15), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    const QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {jan15_2020_close, 100.0, 101.0, 99.0, 100.5, 1e6},
        {jan15_2020_close + 86400, 100.5, 102.0, 100.0, 101.25, 1.1e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2020, 1, 31), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto map = mgr.getBarsMulti({QStringLiteral("SPY")}, QStringLiteral("Day1"),
                                      QStringLiteral("yahoo"), from, to, &refreshed);

    QCOMPARE(nam.requestCount(), 1);
    QVERIFY(!refreshed.isEmpty());

    const QUrl    url(nam.lastRequestUrl());
    QUrlQuery     q(url);
    const qint64  p1 = q.queryItemValue(QStringLiteral("period1")).toLongLong();
    const qint64  p2 = q.queryItemValue(QStringLiteral("period2")).toLongLong();

    const qint64 jan31UtcMidnight =
        QDateTime(QDate(2020, 1, 31), QTime(0, 0), QTimeZone::utc()).toSecsSinceEpoch();

    QVERIFY2(p1 < jan31UtcMidnight,
             "Yahoo period1 must start before the last day of the range (regression: degenerate "
             "multi-symbol trailing window used only the end date).");
    QVERIFY2(p2 > p1 + 86400 * 3, "Yahoo chart window must cover more than a single day for this gap.");

    QVERIFY(!map.value(QStringLiteral("SPY")).isEmpty());

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::getBars_withStrategyAssetMapUsesResolverPath()
{
    const QString conn = QStringLiteral("hist_cache_tst3_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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

    QHash<QString, QVariantMap> strat;
    QVariantMap                 spyEntry;
    spyEntry[AssetFields::Position::ClassificationOverride] = QStringLiteral("Forex");
    strat.insert(QStringLiteral("SPY"), spyEntry);

    QString refreshed;
    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, &refreshed, strat);

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

void TestHistoricalDataManagerCache::getBarsMulti_uncachedSymbols_mockYahooUsesBatchedFetch()
{
    const QString conn =
        QStringLiteral("hist_cache_batch_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
    }

    MockNetworkAccessManager nam;
    QStringList symbols;
    constexpr int kSymbolCount = 7;
    for (int i = 0; i < kSymbolCount; ++i) {
        const QString s = QStringLiteral("TST%1").arg(i);
        symbols.append(s);
        nam.addSymbolResponse(s, twoDayMockYahooChartJson(s));
    }

    HistoricalDataManager mgr(conn, &nam);
    HistoricalDataManager::Config cfg;
    cfg.yahooFetchBatchSize = 3;
    cfg.yahooFetchTimeoutMs = 30000;
    mgr.setConfig(cfg);

    // Window must match mock bars (two daily closes) so after the first fetch the cache fully
    // covers [from, effectiveTo] and a second getBarsMulti does not refetch (Yahoo Day1 gap logic).
    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2024, 1, 3), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto map = mgr.getBarsMulti(symbols, QStringLiteral("Day1"), QStringLiteral("yahoo"), from, to, &refreshed);

    QCOMPARE(nam.requestCount(), kSymbolCount);
    QVERIFY(!refreshed.isEmpty());
    QCOMPARE(map.size(), kSymbolCount);
    for (const QString& s : symbols) {
        QVERIFY2(map.value(s).size() >= 2, qPrintable(s));
    }

    QString r2;
    const auto map2 = mgr.getBarsMulti(symbols, QStringLiteral("Day1"), QStringLiteral("yahoo"), from, to, &r2);
    QCOMPARE(nam.requestCount(), kSymbolCount);
    QVERIFY(r2.isEmpty());
    QCOMPARE(map2.size(), kSymbolCount);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}
