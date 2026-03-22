#include "Backtest/BacktestRunPersistence.h"
#include "Backtest/BacktestDataTypes.h"

namespace Backtest::Persistence {

QString resolvedStrategyIdForPersistence(const BacktestRunConfig& config,
                                         const QString& runIdForFallback)
{
    if (!config.strategyId.isEmpty())
        return config.strategyId;
    if (!config.catalogStrategyId.isEmpty())
        return config.catalogStrategyId;
    if (!config.strategyDefId.isEmpty())
        return config.strategyDefId;
    if (!config.scopeRefId.isEmpty())
        return config.scopeRefId;
    if (!runIdForFallback.isEmpty())
        return QStringLiteral("run:") + runIdForFallback;
    return QStringLiteral("unscoped");
}

} // namespace Backtest::Persistence
