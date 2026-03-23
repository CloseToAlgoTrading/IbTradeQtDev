#ifndef TST_MARKET_SESSION_UTILS_H
#define TST_MARKET_SESSION_UTILS_H

#include <QObject>
#include <QtTest>

class TestMarketSessionUtils : public QObject {
    Q_OBJECT
private slots:
    void classifyYahooSymbol_marksForexCryptoAndEquity();
    void clampEnd_movesNyWeekendToLastWeekday();
    void weekendOnlyChartWindow_singleSundayUtc();
    void allEquity_requiresNoCryptoOrForex();
    void yahooUsDailySession_usesProviderMetadataWhenDbOpen();
};

#endif
