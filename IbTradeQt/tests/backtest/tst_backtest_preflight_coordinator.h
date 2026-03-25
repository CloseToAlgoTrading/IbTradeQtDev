#ifndef TST_BACKTEST_PREFLIGHT_COORDINATOR_H
#define TST_BACKTEST_PREFLIGHT_COORDINATOR_H

#include <QObject>

class TestBacktestPreFlightCoordinator : public QObject {
    Q_OBJECT
private slots:
    void resolveRunConfigSymbols_explicitUnchanged();
    void runPrepareSync_csv_skipsYahooValidation();
    void runPrepareSync_yahoo_requiresNetworkAccessManager();
    void runPrepareSync_yahoo_mock_ok();
};

#endif
