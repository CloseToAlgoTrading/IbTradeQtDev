#ifndef TST_HISTORICAL_DATA_MANAGER_CACHE_H
#define TST_HISTORICAL_DATA_MANAGER_CACHE_H

#include <QObject>

/// Regression: Yahoo Day1 gap logic must use UTC *dates*, not QDateTime instants, so
/// end-of-day `to` (23:59) does not look like a trailing gap vs session-close bars (~21:00 UTC).
/// Also validates weekend skip + no redundant GET when cache already covers the range.
class TestHistoricalDataManagerCache : public QObject {
    Q_OBJECT
private slots:
    void preloadedRange_endOfDayTo_doesNotTriggerYahooGet();
    void secondIdenticalGetBars_doesNotIncrementRequestCount();
};

#endif
