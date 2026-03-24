#ifndef TST_BACKTEST_REPORT_GOLDEN_H
#define TST_BACKTEST_REPORT_GOLDEN_H

#include <QObject>

class TestBacktestReportGolden : public QObject {
    Q_OBJECT
private slots:
    void reportModelJson_includesExtendedSummaryKeys();
    void templateRenderer_replacesPlaceholder();
    void reportModelJson_hasGeneratorVersion();
};

#endif
