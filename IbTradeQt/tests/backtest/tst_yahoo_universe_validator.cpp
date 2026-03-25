#include "tst_yahoo_universe_validator.h"
#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/YahooUniverseValidator.h"

#include <QtTest>
#include <QDate>
#include <QTimeZone>

using namespace Backtest;

void TestYahooUniverseValidator::validate_successAndFailureSplit()
{
    MockNetworkAccessManager nam;
    const qint64 base =
        QDateTime(QDate(2024, 1, 2), QTime(21, 0), QTimeZone::utc()).toSecsSinceEpoch();
    QVector<std::tuple<qint64, double, double, double, double, double>> rows = {
        {base, 100.0, 101.0, 99.0, 100.5, 1e6},
    };
    nam.addSymbolResponse(QStringLiteral("GOOD"), buildYahooJson(QStringLiteral("GOOD"), rows));
    nam.addErrorResponse(QStringLiteral("BAD"));

    YahooUniverseValidator::Config cfg;
    const auto r = YahooUniverseValidator::validate(
        {QStringLiteral("GOOD"), QStringLiteral("BAD")}, &nam, cfg,
        QDateTime(QDate(2024, 1, 10), QTime(12, 0), Qt::UTC));

    QVERIFY(r.okSymbols.contains(QStringLiteral("GOOD")));
    QVERIFY(!r.okSymbols.contains(QStringLiteral("BAD")));
    QVERIFY(r.failedSymbolErrors.contains(QStringLiteral("BAD")));
}
