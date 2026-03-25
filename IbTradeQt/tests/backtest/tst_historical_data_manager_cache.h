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
    void getBars_withStrategyAssetMapUsesResolverPath();
    /// Empty cache + MockNetworkAccessManager: exercises fetchAndCache Yahoo batching (no real HTTP).
    void getBarsMulti_uncachedSymbols_mockYahooUsesBatchedFetch();
    /// Partial cache with trailing gap: Yahoo GET must use period1 at the first missing day, not a degenerate end-only window.
    void getBarsMulti_trailingGap_yahooPeriodSpansMissingTail();
    
    // Historical Read Policy tests
    void refreshFromSource_fetchesFullRange_notJustGaps();
    void refreshFromSource_writesThrough_andReturnsFetchedData();
    void sourceOnly_doesNotReadOrWriteDB();
    void sourceOnly_returnsFetchedDataOnly();
    void normalizationEquivalence_allPoliciesReturnIdenticalBars();
    void getBarsMulti_partialFailure_transportError_omitsSymbol();
    void getBarsMulti_partialFailure_validEmpty_includesEmptyVector();

    void computeCoveragePlan_day_fullyCached();
    void computeCoveragePlan_minute_partialGap_when_intraday_missing();
    void computeCoveragePlan_tick_partialGap();
};

#endif
