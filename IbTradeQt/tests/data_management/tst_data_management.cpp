#include "data_management/tst_data_management.h"
#include "DataManagement/BacktestMarketDataRepository.h"
#include "DataManagement/CsvBarsExporter.h"
#include "DataManagement/CsvBarsImporter.h"
#include "DataManagement/DataManagementService.h"
#include "DataManagement/DataManagementTypes.h"
#include "DataManagement/DataSourceIdNormalizer.h"
#include "DataManagement/YahooHistoricalBarsCoveragePlanner.h"
#include "DataManagement/ExportFilenameUtils.h"
#include "DB/dbdatatypes.h"
#include "DB/dbquery.h"

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QDate>
#include <QTime>

void TestDataManagement::repository_inventoryAndDelete()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/bars.sqlite");

    DataManagement::BacktestMarketDataRepository repo(QStringLiteral("dm_test_inv"));
    QString err;
    QVERIFY(repo.ensureOpen(path, &err));
    QVERIFY(err.isEmpty());

    QList<DataManagement::HistoricalBarsDataset> inv = repo.loadInventory(&err);
    QVERIFY(err.isEmpty());
    QCOMPARE(inv.size(), 0);

    DbHistoricalBar b;
    b.symbol = QStringLiteral("TEST");
    b.resolution = QStringLiteral("Day1");
    b.dataSourceId = QStringLiteral("csv");
    b.timestamp = QStringLiteral("2024-01-02T09:30:00");
    b.open = 1;
    b.high = 2;
    b.low = 0.5;
    b.close = 1.5;
    b.volume = 1000;

    {
        auto q = query_upsertHistoricalBar(b, repo.connectionName());
        QVERIFY(q.exec());
    }

    inv = repo.loadInventory(&err);
    QVERIFY(err.isEmpty());
    QCOMPARE(inv.size(), 1);
    QCOMPARE(inv[0].rowCount, 1);

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol = QStringLiteral("TEST");
    key.resolution = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("csv");
    QVERIFY(repo.deleteDatasets({key}, &err));
    QVERIFY(err.isEmpty());

    inv = repo.loadInventory(&err);
    QVERIFY(err.isEmpty());
    QCOMPARE(inv.size(), 0);

    repo.close();
}

void TestDataManagement::repository_previewUsesTripleWhere()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/pv.sqlite");

    DataManagement::BacktestMarketDataRepository repo(QStringLiteral("dm_test_pv"));
    QString err;
    QVERIFY(repo.ensureOpen(path, &err));

    for (int i = 0; i < 5; ++i) {
        DbHistoricalBar b;
        b.symbol = QStringLiteral("X");
        b.resolution = QStringLiteral("Day1");
        b.dataSourceId = QStringLiteral("yahoo");
        b.timestamp = QStringLiteral("2024-01-%1T00:00:00").arg(i + 1, 2, 10, QChar(QLatin1Char('0')));
        b.open = b.high = b.low = b.close = i;
        b.volume = 1;
        auto q = query_upsertHistoricalBar(b, repo.connectionName());
        QVERIFY(q.exec());
    }

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol = QStringLiteral("X");
    key.resolution = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("yahoo");

    DataManagement::HistoricalBarsPreview pv;
    QVERIFY(repo.loadPreview(key, 2, 2, &pv, &err));
    QCOMPARE(pv.firstRowsAsc.size(), 2);
    QCOMPARE(pv.lastRowsAsc.size(), 2);
    QVERIFY(pv.firstRowsAsc.first().timestampUtc <= pv.firstRowsAsc.last().timestampUtc);
    QVERIFY(pv.lastRowsAsc.first().timestampUtc <= pv.lastRowsAsc.last().timestampUtc);

    repo.close();
}

void TestDataManagement::csvImport_parseAndNormalize()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString csvPath = tmp.path() + QStringLiteral("/in.csv");
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << QStringLiteral("symbol,timestamp,open,high,low,close,volume\n");
        out << QStringLiteral("ZZZ,2024-06-01T12:00:00,1,2,0.5,1.5,100\n");
    }

    auto r = DataManagement::parseCsvBarsForImport(csvPath, QStringLiteral("Day1"),
                                                    QStringLiteral("csv"));
    QVERIFY(r.ok);
    QCOMPARE(r.bars.size(), 1);
    QCOMPARE(r.bars[0].symbol, QStringLiteral("ZZZ"));

    const QString bad = DataManagement::normalizeDataSourceId(QStringLiteral("BAD ID!"));
    QVERIFY(bad.isEmpty());
    QCOMPARE(DataManagement::normalizeDataSourceId(QStringLiteral("  CSV  ")), QStringLiteral("csv"));
}

void TestDataManagement::exportFilename_sanitizes()
{
    const QString b = DataManagement::buildExportBasename(QStringLiteral("BRK/B"), QStringLiteral("Day1"),
                                                          QStringLiteral("csv"));
    QVERIFY(b.contains(QStringLiteral("BRK")));
    QVERIFY(b.endsWith(QStringLiteral(".csv")));
}

void TestDataManagement::csvExport_roundTrip_parseMatchesDbBars()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/roundtrip.sqlite");

    DataManagement::BacktestMarketDataRepository repo(QStringLiteral("dm_test_roundtrip"));
    QString err;
    QVERIFY(repo.ensureOpen(path, &err));
    QVERIFY(err.isEmpty());

    QList<DbHistoricalBar> originals;
    for (int i = 0; i < 3; ++i) {
        DbHistoricalBar b;
        b.symbol = QStringLiteral("RTRIP");
        b.resolution = QStringLiteral("Day1");
        b.dataSourceId = QStringLiteral("csv");
        b.timestamp = QStringLiteral("2024-02-%1T15:30:00Z").arg(i + 1, 2, 10, QChar(QLatin1Char('0')));
        b.open = 1.0 + i;
        b.high = 2.0 + i;
        b.low = 0.5 + i;
        b.close = 1.5 + i;
        b.volume = 100 * (i + 1);
        originals.append(b);
        auto q = query_upsertHistoricalBar(b, repo.connectionName());
        QVERIFY(q.exec());
    }

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol = QStringLiteral("RTRIP");
    key.resolution = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("csv");

    const QString csvPath = tmp.path() + QStringLiteral("/exported.csv");
    QVERIFY(DataManagement::exportDatasetToFile(repo.connectionName(), key, csvPath, &err));
    QVERIFY(err.isEmpty());

    auto parsed = DataManagement::parseCsvBarsForImport(csvPath, QStringLiteral("Day1"),
                                                          QStringLiteral("csv"));
    QVERIFY(parsed.ok);
    QCOMPARE(parsed.bars.size(), originals.size());

    for (int i = 0; i < originals.size(); ++i) {
        QCOMPARE(parsed.bars[i].symbol, originals[i].symbol);
        QCOMPARE(parsed.bars[i].resolution, originals[i].resolution);
        QCOMPARE(parsed.bars[i].dataSourceId, originals[i].dataSourceId);
        QCOMPARE(parsed.bars[i].timestamp, originals[i].timestamp);
        QVERIFY(qFuzzyCompare(parsed.bars[i].open, originals[i].open));
        QVERIFY(qFuzzyCompare(parsed.bars[i].high, originals[i].high));
        QVERIFY(qFuzzyCompare(parsed.bars[i].low, originals[i].low));
        QVERIFY(qFuzzyCompare(parsed.bars[i].close, originals[i].close));
        QVERIFY(qFuzzyCompare(parsed.bars[i].volume, originals[i].volume));
    }

    repo.close();
}

void TestDataManagement::service_asyncContract_importInventoryAndErrors()
{
    // One service instance avoids repeated worker/thread teardown noise in QTest.
    DataManagement::DataManagementService svc;

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // 1) Import (worker transaction + upsert) then inventory lists the dataset
    {
        const QString dbPath = tmp.path() + QStringLiteral("/svc.sqlite");
        const QString csvPath = tmp.path() + QStringLiteral("/in.csv");
        {
            QFile f(csvPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream out(&f);
            out << QStringLiteral("symbol,timestamp,open,high,low,close,volume\n");
            out << QStringLiteral("SVC,2024-03-01T12:00:00Z,1,2,0.5,1.5,99\n");
        }

        QSignalSpy spyImport(&svc, &DataManagement::DataManagementService::barsImportFinished);
        const quint64 opImport = svc.requestImport(dbPath, csvPath, QStringLiteral("Day1"),
                                                   QStringLiteral("csv"));
        QVERIFY(spyImport.wait(20000));
        QCOMPARE(spyImport.count(), 1);
        {
            const QList<QVariant> args = spyImport.takeFirst();
            QCOMPARE(args.at(0).toULongLong(), opImport);
            const auto ir = args.at(1).value<DataManagement::ImportResult>();
            QCOMPARE(ir.rowsParsed, 1);
            QCOMPARE(ir.rowsUpserted, 1);
        }

        QSignalSpy spyInv(&svc, &DataManagement::DataManagementService::barsInventoryLoaded);
        const quint64 opInv = svc.requestBarsInventory(dbPath);
        QVERIFY(spyInv.wait(20000));
        QCOMPARE(spyInv.count(), 1);
        {
            const QList<QVariant> args = spyInv.takeFirst();
            QCOMPARE(args.at(0).toULongLong(), opInv);
            const auto rows = args.at(1).value<QList<DataManagement::HistoricalBarsDataset>>();
            QCOMPARE(rows.size(), 1);
            QCOMPARE(rows[0].symbol, QStringLiteral("SVC"));
            QCOMPARE(rows[0].resolution, QStringLiteral("Day1"));
            QCOMPARE(rows[0].dataSourceId, QStringLiteral("csv"));
            QCOMPARE(rows[0].rowCount, 1);
        }
    }

    // 2) Import invalid resolution: DataManagementError fields
    {
        const QString dbPath = tmp.path() + QStringLiteral("/e.sqlite");
        const QString csvPath = tmp.path() + QStringLiteral("/badres.csv");
        {
            QFile f(csvPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
            f.write("x");
        }
        QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsImportFailed);
        const quint64 id = svc.requestImport(dbPath, csvPath, QStringLiteral("INVALID_RES"),
                                             QStringLiteral("csv"));
        QVERIFY(spyFail.wait(20000));
        QCOMPARE(spyFail.count(), 1);
        const DataManagement::DataManagementError err =
            spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
        QCOMPARE(err.operation, DataManagement::DataManagementOperation::Import);
        QCOMPARE(err.operationId, id);
        QVERIFY(err.message.contains(QStringLiteral("Invalid resolution")));
        QCOMPARE(err.path, csvPath);
    }

    // 3) Missing CSV file
    {
        const QString dbPath = tmp.path() + QStringLiteral("/e2.sqlite");
        const QString csvPath = tmp.path() + QStringLiteral("/missing.csv");
        QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsImportFailed);
        const quint64 id = svc.requestImport(dbPath, csvPath, QStringLiteral("Day1"),
                                             QStringLiteral("csv"));
        QVERIFY(spyFail.wait(20000));
        QCOMPARE(spyFail.count(), 1);
        const DataManagement::DataManagementError err =
            spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
        QCOMPARE(err.operation, DataManagement::DataManagementOperation::Import);
        QCOMPARE(err.operationId, id);
        QVERIFY(err.message.contains(QStringLiteral("Cannot open")));
        QCOMPARE(err.path, csvPath);
    }

    // 4) Inventory: path is a directory (SQLite open fails)
    {
        QVERIFY(QDir(tmp.path()).mkdir(QStringLiteral("isdir")));
        const QString dbPath = tmp.path() + QStringLiteral("/isdir");
        QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsInventoryFailed);
        const quint64 id = svc.requestBarsInventory(dbPath);
        QVERIFY(spyFail.wait(20000));
        QCOMPARE(spyFail.count(), 1);
        const DataManagement::DataManagementError err =
            spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
        QCOMPARE(err.operation, DataManagement::DataManagementOperation::Inventory);
        QCOMPARE(err.operationId, id);
        QCOMPARE(err.path, dbPath);
        QVERIFY(!err.message.isEmpty());
    }
}

void TestDataManagement::yahooCoveragePlanner_tableDriven_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::newRow("empty") << QStringLiteral("empty");
    QTest::newRow("prefix") << QStringLiteral("prefix");
    QTest::newRow("suffix") << QStringLiteral("suffix");
    QTest::newRow("fully_cached") << QStringLiteral("fully_cached");
}

void TestDataManagement::yahooCoveragePlanner_tableDriven()
{
    QFETCH(QString, scenario);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/pl_td.sqlite");

    DataManagement::BacktestMarketDataRepository repo(QStringLiteral("dm_pl_td"));
    QString err;
    QVERIFY(repo.ensureOpen(path, &err));

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol       = QStringLiteral("ZZ");
    key.resolution   = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("yahoo");

    auto insertBar = [&](const QDate& d) {
        DbHistoricalBar b;
        b.symbol       = key.symbol;
        b.resolution   = key.resolution;
        b.dataSourceId = key.dataSourceId;
        b.timestamp    = QDateTime(d, QTime(21, 0), Qt::UTC).toString(Qt::ISODate);
        b.open = b.high = b.low = b.close = 1.0;
        b.volume = 100;
        auto q = query_upsertHistoricalBar(b, repo.connectionName());
        QVERIFY(q.exec());
    };

    if (scenario == QStringLiteral("prefix")) {
        for (const QDate& d : {QDate(2024, 6, 1), QDate(2024, 12, 31)})
            insertBar(d);
    } else if (scenario == QStringLiteral("suffix")) {
        for (const QDate& d : {QDate(2024, 1, 1), QDate(2024, 6, 30)})
            insertBar(d);
    } else if (scenario == QStringLiteral("fully_cached")) {
        for (const QDate& d : {QDate(2024, 1, 1), QDate(2024, 12, 31)})
            insertBar(d);
    }

    const QDateTime fromUtc = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    const QDateTime toUtc   = QDateTime(QDate(2024, 12, 31), QTime(23, 59, 59), Qt::UTC);

    DataManagement::YahooHistoricalBarsCoveragePlanner planner;
    const auto plan = planner.plan(key, fromUtc, toUtc, repo.connectionName());

    if (scenario == QStringLiteral("empty")) {
        QCOMPARE(plan.segments.size(), 1);
        QCOMPARE(plan.segments[0].kind, DataManagement::SyncCoverageSegmentKind::Full);
        QVERIFY(!plan.hasPreviousRange);
    } else if (scenario == QStringLiteral("prefix")) {
        QCOMPARE(plan.segments.size(), 1);
        QCOMPARE(plan.segments[0].kind, DataManagement::SyncCoverageSegmentKind::Prefix);
        QVERIFY(plan.hasPreviousRange);
    } else if (scenario == QStringLiteral("suffix")) {
        QCOMPARE(plan.segments.size(), 1);
        QCOMPARE(plan.segments[0].kind, DataManagement::SyncCoverageSegmentKind::Suffix);
        QVERIFY(plan.hasPreviousRange);
    } else if (scenario == QStringLiteral("fully_cached")) {
        QCOMPARE(plan.segments.size(), 0);
    }

    repo.close();
}

void TestDataManagement::coverageSync_requestSyncBarsCoverage_invalidRange_emitsFailed()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = tmp.path() + QStringLiteral("/cov_bad.sqlite");

    DataManagement::DataManagementService svc;
    QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsCoverageSyncFailed);

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol       = QStringLiteral("X");
    key.resolution   = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("yahoo");

    const QDateTime t = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    const quint64 id  = svc.requestSyncBarsCoverage(dbPath, key, t, t);
    QCOMPARE(spyFail.count(), 1);
    const DataManagement::DataManagementError e =
        spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
    QCOMPARE(e.operation, DataManagement::DataManagementOperation::CoverageSync);
    QCOMPARE(e.operationId, id);
    QVERIFY(!e.message.isEmpty());
}

void TestDataManagement::coverageSync_nonYahoo_emitsFailed_noDbWrites()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = tmp.path() + QStringLiteral("/cov_nony.sqlite");

    {
        DataManagement::BacktestMarketDataRepository seed(QStringLiteral("dm_cov_nony_seed"));
        QString err;
        QVERIFY(seed.ensureOpen(dbPath, &err));
        DbHistoricalBar b;
        b.symbol       = QStringLiteral("ONLYCSV");
        b.resolution   = QStringLiteral("Day1");
        b.dataSourceId = QStringLiteral("csv");
        b.timestamp    = QStringLiteral("2024-01-02T09:30:00");
        b.open = b.high = b.low = b.close = 1.0;
        b.volume = 10;
        QVERIFY(query_upsertHistoricalBar(b, seed.connectionName()).exec());
        QSqlQuery q(QSqlDatabase::database(seed.connectionName()));
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM HistoricalBars")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        seed.close();
    }

    DataManagement::DataManagementService svc;
    QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsCoverageSyncFailed);

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol       = QStringLiteral("ONLYCSV");
    key.resolution   = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("csv");

    const QDateTime fromUtc = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    const QDateTime toUtc   = QDateTime(QDate(2024, 12, 31), QTime(0, 0), Qt::UTC);
    const quint64 id        = svc.requestSyncBarsCoverage(dbPath, key, fromUtc, toUtc);
    QVERIFY(spyFail.wait(10000));
    QCOMPARE(spyFail.count(), 1);
    const DataManagement::DataManagementError e =
        spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
    QCOMPARE(e.operation, DataManagement::DataManagementOperation::CoverageSync);
    QCOMPARE(e.operationId, id);
    QVERIFY(e.message.contains(QStringLiteral("Yahoo")));

    {
        DataManagement::BacktestMarketDataRepository verify(QStringLiteral("dm_cov_nony_verify"));
        QString err;
        QVERIFY(verify.ensureOpen(dbPath, &err));
        QSqlQuery q(QSqlDatabase::database(verify.connectionName()));
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM HistoricalBars")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        verify.close();
    }
}

void TestDataManagement::coverageSync_yahooFullyCached_zeroFetch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = tmp.path() + QStringLiteral("/cov_full.sqlite");

    {
        DataManagement::BacktestMarketDataRepository seed(QStringLiteral("dm_cov_full_seed"));
        QString err;
        QVERIFY(seed.ensureOpen(dbPath, &err));
        DataManagement::HistoricalBarsDatasetKey key;
        key.symbol       = QStringLiteral("ZZ");
        key.resolution   = QStringLiteral("Day1");
        key.dataSourceId = QStringLiteral("yahoo");
        for (const QDate& d : {QDate(2024, 1, 1), QDate(2024, 12, 31)}) {
            DbHistoricalBar b;
            b.symbol       = key.symbol;
            b.resolution   = key.resolution;
            b.dataSourceId = key.dataSourceId;
            b.timestamp    = QDateTime(d, QTime(21, 0), Qt::UTC).toString(Qt::ISODate);
            b.open = b.high = b.low = b.close = 1.0;
            b.volume = 100;
            QVERIFY(query_upsertHistoricalBar(b, seed.connectionName()).exec());
        }
        seed.close();
    }

    DataManagement::DataManagementService svc;
    QSignalSpy spyOk(&svc, &DataManagement::DataManagementService::barsCoverageSyncFinished);

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol       = QStringLiteral("ZZ");
    key.resolution   = QStringLiteral("Day1");
    key.dataSourceId = QStringLiteral("yahoo");

    const QDateTime fromUtc = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    const QDateTime toUtc   = QDateTime(QDate(2024, 12, 31), QTime(23, 59, 59), Qt::UTC);
    const quint64 id        = svc.requestSyncBarsCoverage(dbPath, key, fromUtc, toUtc);
    QVERIFY(spyOk.wait(20000));
    QCOMPARE(spyOk.count(), 1);
    const QList<QVariant> args = spyOk.takeFirst();
    QCOMPARE(args.at(0).toULongLong(), id);
    const auto res = args.at(1).value<DataManagement::SyncCoverageResult>();
    QCOMPARE(res.segments.size(), 0);
    QCOMPARE(res.barsFetched, qint64(0));
    QCOMPARE(res.rowsUpserted, qint64(0));
    QVERIFY(res.hasResultingRange);
}

void TestDataManagement::coverageSync_yahooMin5_rejects()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = tmp.path() + QStringLiteral("/cov_min5.sqlite");

    DataManagement::DataManagementService svc;
    QSignalSpy spyFail(&svc, &DataManagement::DataManagementService::barsCoverageSyncFailed);

    DataManagement::HistoricalBarsDatasetKey key;
    key.symbol       = QStringLiteral("X");
    key.resolution   = QStringLiteral("Min5");
    key.dataSourceId = QStringLiteral("yahoo");

    const QDateTime fromUtc = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    const QDateTime toUtc   = QDateTime(QDate(2024, 12, 31), QTime(0, 0), Qt::UTC);
    const quint64 id        = svc.requestSyncBarsCoverage(dbPath, key, fromUtc, toUtc);
    QVERIFY(spyFail.wait(10000));
    QCOMPARE(spyFail.count(), 1);
    const DataManagement::DataManagementError e =
        spyFail.takeFirst().at(1).value<DataManagement::DataManagementError>();
    QCOMPARE(e.operation, DataManagement::DataManagementOperation::CoverageSync);
    QCOMPARE(e.operationId, id);
    QVERIFY(e.message.contains(QStringLiteral("Day1")));
}
