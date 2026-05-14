#include "tst_historical_data_manager_cache.h"

#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/HistoricalDataManager.h"
#include "Pipeline/HistoricalReadPolicy.h"
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

// ============================================================================
// Historical Read Policy Tests
// ============================================================================

void TestHistoricalDataManagerCache::refreshFromSource_fetchesFullRange_notJustGaps()
{
    const QString conn = QStringLiteral("refresh_full_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        insertSpyDailyRange(conn, QDate(2024, 1, 2), QDate(2024, 1, 3));
    }

    MockNetworkAccessManager nam;
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 200.0, 201.0, 199.0, 200.5, 2e6},
        {base + 86400, 200.5, 202.0, 200.0, 201.25, 2.1e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 3), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, &refreshed, {},
                                  Pipeline::HistoricalReadPolicy::RefreshFromSource);

    QCOMPARE(nam.requestCount(), 1);
    QVERIFY(!refreshed.isEmpty());
    QCOMPARE(bars.size(), 2);
    QCOMPARE(bars[0].close, 200.5);
    QCOMPARE(bars[1].close, 201.25);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::refreshFromSource_writesThrough_andReturnsFetchedData()
{
    const QString conn = QStringLiteral("refresh_wt_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        insertSpyDailyRange(conn, QDate(2024, 1, 2), QDate(2024, 1, 3));
    }

    MockNetworkAccessManager nam;
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 300.0, 301.0, 299.0, 300.5, 3e6},
        {base + 86400, 300.5, 302.0, 300.0, 301.25, 3.1e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 3), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, &refreshed, {},
                                  Pipeline::HistoricalReadPolicy::RefreshFromSource);

    QCOMPARE(bars.size(), 2);
    QCOMPARE(bars[0].close, 300.5);
    QCOMPARE(bars[1].close, 301.25);

    QSqlDatabase db = QSqlDatabase::database(conn);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT close FROM HistoricalBars WHERE symbol='SPY' ORDER BY timestamp"));
    QVERIFY(q.exec());
    
    QVector<double> dbCloses;
    while (q.next()) {
        dbCloses.append(q.value(0).toDouble());
    }
    
    QCOMPARE(dbCloses.size(), 2);
    QCOMPARE(dbCloses[0], 300.5);
    QCOMPARE(dbCloses[1], 301.25);

    QString r2;
    const auto bars2 = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                   QStringLiteral("yahoo"), from, to, &r2, {},
                                   Pipeline::HistoricalReadPolicy::PreferCache);
    
    QVERIFY(r2.isEmpty());
    QCOMPARE(bars2.size(), 2);
    QCOMPARE(bars2[0].close, 300.5);
    QCOMPARE(bars2[1].close, 301.25);

    {
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::sourceOnly_doesNotReadOrWriteDB()
{
    const QString conn = QStringLiteral("source_only_db_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 400.0, 401.0, 399.0, 400.5, 4e6},
        {base + 86400, 400.5, 402.0, 400.0, 401.25, 4.1e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 3), QTime(23, 59, 59), QTimeZone::utc());

    QString refreshed;
    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, &refreshed, {},
                                  Pipeline::HistoricalReadPolicy::SourceOnly);

    QCOMPARE(nam.requestCount(), 1);
    QVERIFY(!refreshed.isEmpty());
    QCOMPARE(bars.size(), 2);

    QSqlDatabase db = QSqlDatabase::database(conn);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM HistoricalBars WHERE symbol='SPY'"));
    QVERIFY(q.exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 0);

    {
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::sourceOnly_returnsFetchedDataOnly()
{
    const QString conn = QStringLiteral("source_only_ret_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        insertSpyDailyRange(conn, QDate(2024, 1, 2), QDate(2024, 1, 3));
    }

    MockNetworkAccessManager nam;
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 500.0, 501.0, 499.0, 500.5, 5e6},
        {base + 86400, 500.5, 502.0, 500.0, 501.25, 5.1e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 3), QTime(23, 59, 59), QTimeZone::utc());

    const auto bars = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                  QStringLiteral("yahoo"), from, to, nullptr, {},
                                  Pipeline::HistoricalReadPolicy::SourceOnly);

    QCOMPARE(bars.size(), 2);
    QCOMPARE(bars[0].close, 500.5);
    QCOMPARE(bars[1].close, 501.25);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::normalizationEquivalence_allPoliciesReturnIdenticalBars()
{
    const QString conn = QStringLiteral("norm_equiv_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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
    const qint64 base = QDateTime(QDate(2024, 1, 5), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base - 172800, 98.0, 99.0, 97.0, 98.5, 0.9e6},
        {base - 86400, 99.0, 100.0, 98.0, 99.5, 0.95e6},
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
        {base + 86400, 100.5, 102.0, 100.0, 101.25, 1.1e6},
        {base + 172800, 101.5, 103.0, 101.0, 102.5, 1.2e6},
    };
    nam.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 4), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 6), QTime(23, 59, 59), QTimeZone::utc());

    const auto barsPrefer = mgr.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                        QStringLiteral("yahoo"), from, to, nullptr, {},
                                        Pipeline::HistoricalReadPolicy::PreferCache);

    MockNetworkAccessManager nam2;
    nam2.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));
    HistoricalDataManager mgr2(conn, &nam2);
    
    const auto barsRefresh = mgr2.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                         QStringLiteral("yahoo"), from, to, nullptr, {},
                                         Pipeline::HistoricalReadPolicy::RefreshFromSource);

    MockNetworkAccessManager nam3;
    nam3.addSymbolResponse(QStringLiteral("SPY"), buildYahooJson(QStringLiteral("SPY"), rows));
    HistoricalDataManager mgr3(conn, &nam3);
    
    const auto barsSource = mgr3.getBars(QStringLiteral("SPY"), QStringLiteral("Day1"),
                                        QStringLiteral("yahoo"), from, to, nullptr, {},
                                        Pipeline::HistoricalReadPolicy::SourceOnly);

    QCOMPARE(barsPrefer.size(), barsRefresh.size());
    QCOMPARE(barsPrefer.size(), barsSource.size());
    QCOMPARE(barsPrefer.size(), 2);

    for (int i = 0; i < barsPrefer.size(); ++i) {
        QCOMPARE(barsPrefer[i].symbol, barsRefresh[i].symbol);
        QCOMPARE(barsPrefer[i].symbol, barsSource[i].symbol);
        
        QCOMPARE(barsPrefer[i].timestamp, barsRefresh[i].timestamp);
        QCOMPARE(barsPrefer[i].timestamp, barsSource[i].timestamp);
        
        QCOMPARE(barsPrefer[i].open, barsRefresh[i].open);
        QCOMPARE(barsPrefer[i].open, barsSource[i].open);
        
        QCOMPARE(barsPrefer[i].high, barsRefresh[i].high);
        QCOMPARE(barsPrefer[i].high, barsSource[i].high);
        
        QCOMPARE(barsPrefer[i].low, barsRefresh[i].low);
        QCOMPARE(barsPrefer[i].low, barsSource[i].low);
        
        QCOMPARE(barsPrefer[i].close, barsRefresh[i].close);
        QCOMPARE(barsPrefer[i].close, barsSource[i].close);
        
        QCOMPARE(barsPrefer[i].volume, barsRefresh[i].volume);
        QCOMPARE(barsPrefer[i].volume, barsSource[i].volume);
    }

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::getBarsMulti_partialFailure_transportError_omitsSymbol()
{
    const QString conn = QStringLiteral("multi_fail_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
    };
    
    nam.addSymbolResponse(QStringLiteral("GOOD"), buildYahooJson(QStringLiteral("GOOD"), rows));
    nam.addErrorResponse(QStringLiteral("BAD"));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 2), QTime(23, 59, 59), QTimeZone::utc());

    const auto map = mgr.getBarsMulti({QStringLiteral("BAD"), QStringLiteral("GOOD")},
                                      QStringLiteral("Day1"), QStringLiteral("yahoo"),
                                      from, to, nullptr, {},
                                      Pipeline::HistoricalReadPolicy::RefreshFromSource);

    QVERIFY(!map.contains(QStringLiteral("BAD")));
    QVERIFY(map.contains(QStringLiteral("GOOD")));
    QCOMPARE(map[QStringLiteral("GOOD")].size(), 1);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::getBarsMulti_partialFailure_validEmpty_includesEmptyVector()
{
    const QString conn = QStringLiteral("multi_empty_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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
    const qint64 base = QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
    };
    
    nam.addSymbolResponse(QStringLiteral("EMPTY"), buildYahooJson(QStringLiteral("EMPTY"), {}));
    nam.addSymbolResponse(QStringLiteral("GOOD"), buildYahooJson(QStringLiteral("GOOD"), rows));

    HistoricalDataManager mgr(conn, &nam);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(0, 0), QTimeZone::utc());
    const QDateTime to = QDateTime(QDate(2024, 1, 2), QTime(23, 59, 59), QTimeZone::utc());

    const auto map = mgr.getBarsMulti({QStringLiteral("EMPTY"), QStringLiteral("GOOD")},
                                      QStringLiteral("Day1"), QStringLiteral("yahoo"),
                                      from, to, nullptr, {},
                                      Pipeline::HistoricalReadPolicy::RefreshFromSource);

    QVERIFY(map.contains(QStringLiteral("EMPTY")));
    QVERIFY(map[QStringLiteral("EMPTY")].isEmpty());
    QVERIFY(map.contains(QStringLiteral("GOOD")));
    QCOMPARE(map[QStringLiteral("GOOD")].size(), 1);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::computeCoveragePlan_day_fullyCached()
{
    const QString conn = QStringLiteral("cov_day_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

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

    HistoricalDataManager mgr(conn, nullptr);

    const QDateTime from = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2020, 1, 10), QTime(23, 59, 59), QTimeZone::utc());

    const auto plan = mgr.computeCoveragePlan({QStringLiteral("SPY")}, QStringLiteral("Day1"),
                                              QStringLiteral("yahoo"), from, to, {});
    QVERIFY(plan.contains(QStringLiteral("SPY")));
    QCOMPARE(plan.value(QStringLiteral("SPY")).status, SymbolCoveragePlanEntry::Status::FullyCached);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::computeCoveragePlan_minute_partialGap_when_intraday_missing()
{
    const QString conn = QStringLiteral("cov_min_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        QSqlQuery q(db);
        q.prepare(QLatin1String(
            "INSERT OR REPLACE INTO HistoricalBars "
            "(symbol, resolution, dataSourceId, timestamp, open, high, low, close, volume) "
            "VALUES ('AMD','Min1','yahoo',:ts,100,101,99,100.5,1e6)"));
        const QDateTime ts(QDate(2024, 1, 2), QTime(14, 0), QTimeZone::utc());
        q.bindValue(QStringLiteral(":ts"), ts.toUTC().toString(Qt::ISODate));
        QVERIFY(q.exec());
        db.close();
    }

    HistoricalDataManager mgr(conn, nullptr);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(9, 30), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2024, 1, 2), QTime(16, 0), QTimeZone::utc());

    const auto plan = mgr.computeCoveragePlan({QStringLiteral("AMD")}, QStringLiteral("Min1"),
                                              QStringLiteral("yahoo"), from, to, {});
    QVERIFY(plan.contains(QStringLiteral("AMD")));
    QCOMPARE(plan.value(QStringLiteral("AMD")).status, SymbolCoveragePlanEntry::Status::PartialGap);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestHistoricalDataManagerCache::computeCoveragePlan_tick_partialGap()
{
    const QString conn = QStringLiteral("cov_tick_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        QSqlQuery q(db);
        q.prepare(QLatin1String(
            "INSERT OR REPLACE INTO HistoricalBars "
            "(symbol, resolution, dataSourceId, timestamp, open, high, low, close, volume) "
            "VALUES ('SPY','Tick','yahoo',:ts,100,101,99,100.5,1e3)"));
        const QDateTime ts(QDate(2024, 1, 2), QTime(14, 0, 0), QTimeZone::utc());
        q.bindValue(QStringLiteral(":ts"), ts.toUTC().toString(Qt::ISODate));
        QVERIFY(q.exec());
        db.close();
    }

    HistoricalDataManager mgr(conn, nullptr);

    const QDateTime from = QDateTime(QDate(2024, 1, 2), QTime(13, 0), QTimeZone::utc());
    const QDateTime to   = QDateTime(QDate(2024, 1, 2), QTime(15, 0), QTimeZone::utc());

    const auto plan = mgr.computeCoveragePlan({QStringLiteral("SPY")}, QStringLiteral("Tick"),
                                              QStringLiteral("yahoo"), from, to, {});
    QVERIFY(plan.contains(QStringLiteral("SPY")));
    QCOMPARE(plan.value(QStringLiteral("SPY")).status, SymbolCoveragePlanEntry::Status::PartialGap);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}
