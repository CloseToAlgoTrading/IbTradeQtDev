#include "tst_backtest_preflight_coordinator.h"
#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/BacktestPreFlightCoordinator.h"
#include "Backtest/BacktestDataTypes.h"
#include "DB/dbquery.h"

#include <QtTest>
#include <QDate>
#include <QSqlDatabase>
#include <QSqlQuery>
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

void TestBacktestPreFlightCoordinator::resolveRunConfigSymbols_explicitUnchanged()
{
    BacktestRunConfig c;
    c.symbols = QStringList{QStringLiteral("ZZZ")};
    QCOMPARE(BacktestPreFlightCoordinator::resolveRunConfigSymbols(c).symbols, c.symbols);
}

void TestBacktestPreFlightCoordinator::runPrepareSync_csv_skipsYahooValidation()
{
    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    const QString conn = QStringLiteral("pf_csv_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);

    BacktestPreFlightCoordinator::PrepareInput in;
    in.dbFilePath                 = dbFile.fileName();
    in.runConfig.dataSourceId     = QStringLiteral("csv");
    in.runConfig.symbols          = QStringList{QStringLiteral("X")};
    in.runConfig.startDate        = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    in.runConfig.endDate          = QDateTime(QDate(2024, 1, 31), QTime(23, 59, 59), Qt::UTC);
    in.networkAccessManager       = nullptr;

    const BacktestPreFlightResult r = BacktestPreFlightCoordinator::runPrepareSync(in);
    QVERIFY(r.ok);
    QVERIFY(r.yahooValidation.failedSymbolErrors.isEmpty());
    QVERIFY(r.yahooValidation.okSymbols.isEmpty());
}

void TestBacktestPreFlightCoordinator::runPrepareSync_yahoo_requiresNetworkAccessManager()
{
    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    const QString conn = QStringLiteral("pf_yahoo_nonam_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);

    BacktestPreFlightCoordinator::PrepareInput in;
    in.dbFilePath             = dbFile.fileName();
    in.runConfig.dataSourceId = QStringLiteral("yahoo");
    in.runConfig.symbols      = QStringList{QStringLiteral("AMD")};
    in.runConfig.startDate    = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    in.runConfig.endDate      = QDateTime(QDate(2024, 1, 31), QTime(23, 59, 59), Qt::UTC);
    in.networkAccessManager   = nullptr;

    const BacktestPreFlightResult r = BacktestPreFlightCoordinator::runPrepareSync(in);
    QVERIFY(!r.ok);
    QVERIFY(!r.errorMessage.isEmpty());
}

void TestBacktestPreFlightCoordinator::runPrepareSync_yahoo_mock_ok()
{
    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    const QString conn = QStringLiteral("pf_yahoo_ok_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        createHistoricalBarsTable(conn);
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);

    MockNetworkAccessManager nam;
    const qint64 base =
        QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
    };
    nam.addSymbolResponse(QStringLiteral("AMD"), buildYahooJson(QStringLiteral("AMD"), rows));

    BacktestPreFlightCoordinator::PrepareInput in;
    in.dbFilePath               = dbFile.fileName();
    in.runConfig.dataSourceId   = QStringLiteral("yahoo");
    in.runConfig.resolution     = QStringLiteral("Day1");
    in.runConfig.symbols        = QStringList{QStringLiteral("AMD")};
    in.runConfig.startDate      = QDateTime(QDate(2024, 1, 1), QTime(0, 0), Qt::UTC);
    in.runConfig.endDate        = QDateTime(QDate(2024, 1, 10), QTime(23, 59, 59), Qt::UTC);
    in.networkAccessManager     = &nam;

    const BacktestPreFlightResult r = BacktestPreFlightCoordinator::runPrepareSync(in);
    QVERIFY(r.ok);
    QVERIFY(!r.yahooValidation.failedSymbolErrors.contains(QStringLiteral("AMD")));
    QVERIFY(r.strategySymbolCoverage.contains(QStringLiteral("AMD")));
    QCOMPARE(r.strategySymbolCoverage.value(QStringLiteral("AMD")).status,
             SymbolCoveragePlanEntry::Status::NeedsFetch);
}
