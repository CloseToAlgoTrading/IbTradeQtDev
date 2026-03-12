#ifndef TST_SCOPE_H
#define TST_SCOPE_H

#include <QtTest>
#include "Pipeline/Scope.h"

class TestScope : public QObject
{
    Q_OBJECT

private slots:
    void scopeEnumValues()
    {
        QCOMPARE(static_cast<int>(Pipeline::Scope::Strategy), 0);
        QCOMPARE(static_cast<int>(Pipeline::Scope::Portfolio), 1);
        QCOMPARE(static_cast<int>(Pipeline::Scope::Account), 2);
    }

    void riskScopeOrderSize()
    {
        const auto& order = Pipeline::riskScopeOrder();
        QCOMPARE(static_cast<int>(order.size()), 3);
    }

    void riskScopeOrderIsStrategyFirst()
    {
        const auto& order = Pipeline::riskScopeOrder();
        QCOMPARE(order[0], Pipeline::Scope::Strategy);
        QCOMPARE(order[1], Pipeline::Scope::Portfolio);
        QCOMPARE(order[2], Pipeline::Scope::Account);
    }

    void riskScopeOrderIsDeterministic()
    {
        const auto& order1 = Pipeline::riskScopeOrder();
        const auto& order2 = Pipeline::riskScopeOrder();

        QCOMPARE(order1.size(), order2.size());
        for (size_t i = 0; i < order1.size(); ++i) {
            QCOMPARE(order1[i], order2[i]);
        }
    }

    void riskScopeOrderReturnsSameReference()
    {
        const auto& ref1 = Pipeline::riskScopeOrder();
        const auto& ref2 = Pipeline::riskScopeOrder();
        QCOMPARE(&ref1, &ref2);
    }

    void allScopesPresent()
    {
        const auto& order = Pipeline::riskScopeOrder();
        bool hasStrategy = false, hasPortfolio = false, hasAccount = false;

        for (const auto& scope : order) {
            if (scope == Pipeline::Scope::Strategy) hasStrategy = true;
            if (scope == Pipeline::Scope::Portfolio) hasPortfolio = true;
            if (scope == Pipeline::Scope::Account) hasAccount = true;
        }

        QVERIFY(hasStrategy);
        QVERIFY(hasPortfolio);
        QVERIFY(hasAccount);
    }
};

#endif // TST_SCOPE_H
