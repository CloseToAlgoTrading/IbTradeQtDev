#include "tst_market_session_utils.h"

#include "Backtest/MarketSessionUtils.h"

#include <QDate>
#include <QTime>
#include <QTimeZone>

using namespace Backtest;

void TestMarketSessionUtils::classifyYahooSymbol_marksForexCryptoAndEquity()
{
    QCOMPARE(classifyYahooSymbol(QStringLiteral("EURUSD=X")), InstrumentKind::Forex);
    QCOMPARE(classifyYahooSymbol(QStringLiteral("BTC-USD")), InstrumentKind::Crypto);
    QCOMPARE(classifyYahooSymbol(QStringLiteral("SPY")), InstrumentKind::EquityUs);
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
    QVERIFY(allSymbolsClassifyAsUsEquity({QStringLiteral("SPY"), QStringLiteral("AMD")}));
    QVERIFY(!allSymbolsClassifyAsUsEquity({QStringLiteral("SPY"), QStringLiteral("BTC-USD")}));
    QVERIFY(!allSymbolsClassifyAsUsEquity({QStringLiteral("SPY"), QStringLiteral("EURUSD=X")}));
}
