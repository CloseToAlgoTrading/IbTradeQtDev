#include "Backtest/InstrumentInference.h"

namespace Backtest {

AssetKind inferAssetKindFromSymbolHeuristic(const QString& symbol)
{
    const QString s = symbol.trimmed();
    if (s.contains(QLatin1Char('-')))
        return AssetKind::Crypto;
    if (s.endsWith(QLatin1String("=X")))
        return AssetKind::Forex;
    return AssetKind::Equity;
}

bool appliesYahooUsCashEquitySessionDaily(AssetKind k)
{
    switch (k) {
    case AssetKind::Forex:
    case AssetKind::Crypto:
    case AssetKind::Unknown:
        return false;
    default:
        return true;
    }
}

} // namespace Backtest
