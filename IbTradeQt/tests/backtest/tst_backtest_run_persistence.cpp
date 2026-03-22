#include "backtest/tst_backtest_run_persistence.h"
#include "Backtest/BacktestRunPersistence.h"
#include "Backtest/BacktestDataTypes.h"

using Backtest::BacktestRunConfig;
using Backtest::Persistence::resolvedStrategyIdForPersistence;

void TestBacktestRunPersistence::resolvedStrategyId_prefersLiveNodeId()
{
    BacktestRunConfig c;
    c.strategyId = QStringLiteral("live-node-uuid");
    c.catalogStrategyId = QStringLiteral("cat-id");
    QCOMPARE(resolvedStrategyIdForPersistence(c, QStringLiteral("run1")), c.strategyId);
}

void TestBacktestRunPersistence::resolvedStrategyId_usesCatalogWhenLiveEmpty()
{
    BacktestRunConfig c;
    c.catalogStrategyId = QStringLiteral("catalog-strategy-uuid");
    QCOMPARE(resolvedStrategyIdForPersistence(c, QStringLiteral("run1")), c.catalogStrategyId);
}

void TestBacktestRunPersistence::resolvedStrategyId_fallsBackToStrategyDefId()
{
    BacktestRunConfig c;
    c.strategyDefId = QStringLiteral("def-only");
    QCOMPARE(resolvedStrategyIdForPersistence(c, QStringLiteral("run1")), c.strategyDefId);
}

void TestBacktestRunPersistence::resolvedStrategyId_fallsBackToScopeRefId()
{
    BacktestRunConfig c;
    c.scopeRefId = QStringLiteral("scope-ref");
    QCOMPARE(resolvedStrategyIdForPersistence(c, QStringLiteral("run1")), c.scopeRefId);
}

void TestBacktestRunPersistence::resolvedStrategyId_fallsBackToRunIdPrefix()
{
    BacktestRunConfig c;
    QCOMPARE(resolvedStrategyIdForPersistence(c, QStringLiteral("abc-123")),
             QStringLiteral("run:abc-123"));
}

void TestBacktestRunPersistence::resolvedStrategyId_allEmpty_usesUnscoped()
{
    BacktestRunConfig c;
    QCOMPARE(resolvedStrategyIdForPersistence(c, QString()), QStringLiteral("unscoped"));
}
