#ifndef TST_BACKTEST_RUN_PERSISTENCE_H
#define TST_BACKTEST_RUN_PERSISTENCE_H

#include <QObject>
#include <QtTest>

class TestBacktestRunPersistence : public QObject {
    Q_OBJECT
private slots:
    void resolvedStrategyId_prefersLiveNodeId();
    void resolvedStrategyId_usesCatalogWhenLiveEmpty();
    void resolvedStrategyId_fallsBackToStrategyDefId();
    void resolvedStrategyId_fallsBackToScopeRefId();
    void resolvedStrategyId_fallsBackToRunIdPrefix();
    void resolvedStrategyId_allEmpty_usesUnscoped();
};

#endif // TST_BACKTEST_RUN_PERSISTENCE_H
