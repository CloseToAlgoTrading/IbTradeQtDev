#ifndef BACKTEST_INSTRUMENTINFERENCE_H
#define BACKTEST_INSTRUMENTINFERENCE_H

#include "Backtest/InstrumentClassification.h"
#include <QString>

namespace Backtest {

/// Single choke point for symbol-shape heuristics (Yahoo-style tickers).
/// Used only from InstrumentMetadataResolver / tests — not from session helpers directly.
AssetKind inferAssetKindFromSymbolHeuristic(const QString& symbol);

/// True when Yahoo US cash equity daily session rules apply (weekend gap skip, clamp).
/// Forex and crypto are excluded; other coarse kinds included.
bool appliesYahooUsCashEquitySessionDaily(AssetKind k);

} // namespace Backtest

#endif
