#ifndef PIPELINE_HISTORICALREADPOLICY_H
#define PIPELINE_HISTORICALREADPOLICY_H

namespace Pipeline {

/// Policy for historical bar retrieval — controls cache consultation and write behavior.
///
/// - PreferCache: Query local store for coverage; fetch only missing gaps from provider;
///   persist fetched bars; return full requested range via merged cache read (existing default).
/// - RefreshFromSource: Bypass cache consultation; fetch full requested range from provider;
///   write-through to persistent store (updates cache for other consumers); return the fetched
///   bars directly after normalization (no DB reread for the response).
/// - SourceOnly: Fetch full range from provider; do not consult or update persistent store for
///   this request; return fetched bars after normalization (ephemeral read).
enum class HistoricalReadPolicy {
    PreferCache,
    RefreshFromSource,
    SourceOnly
};

} // namespace Pipeline

#endif // PIPELINE_HISTORICALREADPOLICY_H
