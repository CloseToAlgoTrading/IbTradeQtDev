#include "tst_market_session_utils.h"

#include "Backtest/InstrumentClassification.h"
#include "Backtest/MarketSessionUtils.h"
#include "DB/dbquery.h"
#include "DB/dbdatatypes.h"

#include <QDate>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QTime>
#include <QTimeZone>
#include <QUuid>

using namespace Backtest;

void TestMarketSessionUtils::classifyYahooSymbol_marksForexCryptoAndEquity()
{
    QCOMPARE(classifyYahooSymbol(QStringLiteral("EURUSD=X")), AssetKind::Forex);
    QCOMPARE(classifyYahooSymbol(QStringLiteral("BTC-USD")), AssetKind::Crypto);
    QCOMPARE(classifyYahooSymbol(QStringLiteral("SPY")), AssetKind::Equity);
}

void TestMarketSessionUtils::clampEnd_movesNyWeekendToLastWeekday()
{
    const QTimeZone ny(QByteArrayLiteral("America/New_York"));
    // Sunday 2026-03-22 15:00 NY → clamp to Friday 2026-03-20 15:00 NY
    const QDateTime endNy(QDate(2026, 3, 22), QTime(15, 0), ny);
    const QDateTime clamped = clampEndDateTimeForUsEquityDaily(endNy.toUTC());
    const QDateTime expectNy(QDate(2026, 3, 20), QTime(15, 0), ny);
    QCOMPARE(clamped.toSecsSinceEpoch(), expectNy.toUTC().toSecsSinceEpoch());
}

void TestMarketSessionUtils::weekendOnlyChartWindow_singleSundayUtc()
{
    // 2026-03-22 00:00 UTC .. 2026-03-23 00:00 UTC (exclusive) — Sunday UTC day only
    const qint64 p1 = QDateTime(QDate(2026, 3, 22), QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
    const qint64 p2 = QDateTime(QDate(2026, 3, 23), QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
    QVERIFY(isWeekendOnlyChartWindowUtcNy(p1, p2));
}

void TestMarketSessionUtils::allEquity_requiresNoCryptoOrForex()
{
    QVERIFY(allSymbolsUseYahooUsCashEquitySessionDaily({QStringLiteral("SPY"), QStringLiteral("AMD")}));
    QVERIFY(!allSymbolsUseYahooUsCashEquitySessionDaily({QStringLiteral("SPY"), QStringLiteral("BTC-USD")}));
    QVERIFY(!allSymbolsUseYahooUsCashEquitySessionDaily({QStringLiteral("SPY"), QStringLiteral("EURUSD=X")}));
}

void TestMarketSessionUtils::yahooUsDailySession_usesProviderMetadataWhenDbOpen()
{
    const QString conn =
        QStringLiteral("mkt_sess_md_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec(QLatin1String(CREATE_TABLE_INSTRUMENT_METADATA)));
        DbInstrumentMetadata row;
        row.providerSymbol = QStringLiteral("SPY");
        row.providerId     = QStringLiteral("yahoo");
        row.assetKind        = QStringLiteral("Forex");
        row.updatedAt        = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        QVERIFY(query_upsertInstrumentMetadata(row, conn).exec());
    }

    QVERIFY(!allSymbolsUseYahooUsCashEquitySessionDaily(conn, QStringLiteral("yahoo"),
                                                         {QStringLiteral("SPY")}, {}));

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);

    QVERIFY(allSymbolsUseYahooUsCashEquitySessionDaily({QStringLiteral("SPY")}));
}
