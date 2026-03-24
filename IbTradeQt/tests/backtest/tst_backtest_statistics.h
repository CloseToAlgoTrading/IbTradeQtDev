#ifndef TST_BACKTEST_STATISTICS_H
#define TST_BACKTEST_STATISTICS_H

#include <QObject>

class TestBacktestStatistics : public QObject {
    Q_OBJECT
private slots:
    void sortino_zeroWhenFlat();
    void profitFactor_zeroTrades();
    void json_hasSchemaVersion();
    void profitFactor_allLosingRoundTrip();
    void exposure_cashVsInvested();
    void monthlyReturns_twoMonths();
    void turnover_positiveWithTrades();
};

#endif
