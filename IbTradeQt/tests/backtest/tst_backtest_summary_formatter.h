#ifndef TST_BACKTEST_SUMMARY_FORMATTER_H
#define TST_BACKTEST_SUMMARY_FORMATTER_H

#include <QObject>

class TestBacktestSummaryFormatter : public QObject {
    Q_OBJECT

private slots:
    void dataQualityLabel_dailyBars();
    void buildRows_noBenchmark_twoColumns();
    void buildRows_withBenchmark_threeColumns();
    void policyForSemanticReport_matchesEvaluationRebalance();
    void equityCurveToPnlSeries_basic();
    void equityCurveToPnlSeries_empty();
};

#endif
