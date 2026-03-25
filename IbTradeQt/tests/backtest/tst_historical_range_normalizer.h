#ifndef TST_HISTORICAL_RANGE_NORMALIZER_H
#define TST_HISTORICAL_RANGE_NORMALIZER_H

#include <QObject>

class TestHistoricalRangeNormalizer : public QObject {
    Q_OBJECT
private slots:
    void normalize_day1_usesUtcCalendarBounds();
    void normalize_min1_alignsToBarPeriod();
    void normalize_tick_preservesFullTimestamp();
};

#endif
