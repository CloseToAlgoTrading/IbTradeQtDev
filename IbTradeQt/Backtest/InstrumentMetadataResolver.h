#ifndef BACKTEST_INSTRUMENTMETADATARESOLVER_H
#define BACKTEST_INSTRUMENTMETADATARESOLVER_H

#include "Backtest/InstrumentClassification.h"
#include <QVariantMap>

namespace Backtest {

// Instrument metadata resolution — architecture summary
// ------------------------------------------------------
// - Provider truth: InstrumentMetadata rows (per providerSymbol + providerId), written only by
//   provider adapters (Yahoo chart meta, IB contract, etc.). Never mix strategy overrides here.
// - Strategy overrides: assetList per symbol (classificationOverride, sessionPolicy) — same map
//   serialized as BacktestRunConfig.assetListJson for HistoricalDataManager worker runs.
// - EffectiveInstrumentProfile: single output type; precedence override > DB row > heuristic.
// - All classification heuristics live in InstrumentInference / this resolver — not in UI or
//   MarketSessionUtils except via shared helpers (no duplicate symbol rules).

/// Single implementation of classification resolution (provider row + override + heuristic).
class InstrumentMetadataResolver {
public:
    /// \a strategyAssetEntry is assetList[symbol] for that symbol (may be empty).
    static EffectiveInstrumentProfile resolve(const QString& dbConnectionName,
                                              const QString& rawSymbol,
                                              const QString& providerId,
                                              const QVariantMap& strategyAssetEntry);
};

} // namespace Backtest

#endif
