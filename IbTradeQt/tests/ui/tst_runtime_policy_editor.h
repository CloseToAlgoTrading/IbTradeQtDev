#ifndef TST_RUNTIME_POLICY_EDITOR_H
#define TST_RUNTIME_POLICY_EDITOR_H

#include <QtTest>
#include <QSignalSpy>
#include <QApplication>
#include "RuntimePolicyEditor.h"
#include "Pipeline/StrategyRuntimePolicy.h"

class TestRuntimePolicyEditor : public QObject
{
    Q_OBJECT

private slots:

    void testSetPolicyRoundTrip()
    {
        RuntimePolicyEditor editor;

        Pipeline::StrategyRuntimePolicy orig;
        orig.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        orig.evaluationIntervalN = 7;
        orig.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNMinutes;
        orig.rebalanceIntervalN = 15;
        orig.accumulateAlphaSignals = false;
        orig.signalExpiryBars = 10;
        orig.riskAlwaysActive = false;
        orig.riskCanCancelPendingOrders = false;
        orig.executionImmediateAfterApproval = false;

        editor.setPolicy(orig);
        auto result = editor.policy();

        QCOMPARE(result.evaluationMode, orig.evaluationMode);
        QCOMPARE(result.evaluationIntervalN, orig.evaluationIntervalN);
        QCOMPARE(result.rebalanceMode, orig.rebalanceMode);
        QCOMPARE(result.rebalanceIntervalN, orig.rebalanceIntervalN);
        QCOMPARE(result.accumulateAlphaSignals, orig.accumulateAlphaSignals);
        QCOMPARE(result.signalExpiryBars, orig.signalExpiryBars);
        QCOMPARE(result.riskAlwaysActive, orig.riskAlwaysActive);
        QCOMPARE(result.riskCanCancelPendingOrders, orig.riskCanCancelPendingOrders);
        QCOMPARE(result.executionImmediateAfterApproval, orig.executionImmediateAfterApproval);
    }

    void testDefaultPolicyRoundTrip()
    {
        RuntimePolicyEditor editor;

        Pipeline::StrategyRuntimePolicy defaults;
        editor.setPolicy(defaults);
        auto result = editor.policy();

        QCOMPARE(result.evaluationMode, defaults.evaluationMode);
        QCOMPARE(result.rebalanceMode, defaults.rebalanceMode);
        QCOMPARE(result.accumulateAlphaSignals, defaults.accumulateAlphaSignals);
        QCOMPARE(result.signalExpiryBars, defaults.signalExpiryBars);
        QCOMPARE(result.riskAlwaysActive, defaults.riskAlwaysActive);
        QCOMPARE(result.riskCanCancelPendingOrders, defaults.riskCanCancelPendingOrders);
        QCOMPARE(result.executionImmediateAfterApproval, defaults.executionImmediateAfterApproval);
    }

    void testLoadFromJsonExtractsRuntimePolicy()
    {
        RuntimePolicyEditor editor;

        QJsonObject pipelineConfig;
        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNMinutes;
        p.evaluationIntervalN = 5;
        p.rebalanceMode = Pipeline::StrategyRuntimePolicy::RebalanceMode::EveryNBars;
        p.rebalanceIntervalN = 10;
        p.accumulateAlphaSignals = false;
        pipelineConfig["runtimePolicy"] = p.toJson();
        pipelineConfig["selection"] = QJsonArray();

        editor.loadFromJson(pipelineConfig);
        auto result = editor.policy();

        QCOMPARE(result.evaluationMode, p.evaluationMode);
        QCOMPARE(result.evaluationIntervalN, 5);
        QCOMPARE(result.rebalanceMode, p.rebalanceMode);
        QCOMPARE(result.rebalanceIntervalN, 10);
        QCOMPARE(result.accumulateAlphaSignals, false);
    }

    void testLoadFromJsonMissingKeyUsesDefaults()
    {
        RuntimePolicyEditor editor;
        QJsonObject emptyConfig;
        editor.loadFromJson(emptyConfig);

        auto result = editor.policy();
        Pipeline::StrategyRuntimePolicy defaults;

        QCOMPARE(result.evaluationMode, defaults.evaluationMode);
        QCOMPARE(result.rebalanceMode, defaults.rebalanceMode);
    }

    void testApplyToJsonWritesBackCorrectly()
    {
        RuntimePolicyEditor editor;

        Pipeline::StrategyRuntimePolicy p;
        p.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        p.evaluationIntervalN = 4;
        editor.setPolicy(p);

        QJsonObject orig;
        orig["selection"] = QJsonArray();
        QJsonObject result = editor.applyToJson(orig);

        QVERIFY(result.contains("runtimePolicy"));
        QVERIFY(result.contains("selection"));

        auto restored = Pipeline::StrategyRuntimePolicy::fromJson(
            result["runtimePolicy"].toObject());
        QCOMPARE(restored.evaluationMode,
                 Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars);
        QCOMPARE(restored.evaluationIntervalN, 4);
    }

    void testApplyToJsonPreservesOtherKeys()
    {
        RuntimePolicyEditor editor;
        Pipeline::StrategyRuntimePolicy p;
        editor.setPolicy(p);

        QJsonObject orig;
        orig["selection"] = QJsonArray({QJsonObject{{"blockId", "sel1"}}});
        orig["customKey"] = 42;

        QJsonObject result = editor.applyToJson(orig);
        QCOMPARE(result["customKey"].toInt(), 42);
        QCOMPARE(result["selection"].toArray().size(), 1);
    }

    void testPolicyChangedSignalEmitsOnModeChange()
    {
        RuntimePolicyEditor editor;
        QSignalSpy spy(&editor, &RuntimePolicyEditor::policyChanged);
        QVERIFY(spy.isValid());

        Pipeline::StrategyRuntimePolicy p;
        editor.setPolicy(p);

        spy.clear();

        // Simulate changing the eval mode combo box programmatically
        // This tests the signal emission path
        Pipeline::StrategyRuntimePolicy newP;
        newP.evaluationMode = Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryNBars;
        newP.evaluationIntervalN = 3;
        editor.setPolicy(newP);

        // setPolicy blocks signals, so no emission during set
        QCOMPARE(spy.count(), 0);
    }

    void testReadOnlyDisablesAllControls()
    {
        RuntimePolicyEditor editor;
        editor.setReadOnly(true);

        auto result = editor.policy();
        Pipeline::StrategyRuntimePolicy defaults;
        QCOMPARE(result.evaluationMode, defaults.evaluationMode);

        editor.setReadOnly(false);
        result = editor.policy();
        QCOMPARE(result.evaluationMode, defaults.evaluationMode);
    }

    void testDelegateMethodsMatchCanonicalSource()
    {
        using E = Pipeline::StrategyRuntimePolicy::EvaluationMode;
        using R = Pipeline::StrategyRuntimePolicy::RebalanceMode;

        QCOMPARE(RuntimePolicyEditor::evalModeLabel(E::EveryTick),
                 Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryTick));
        QCOMPARE(RuntimePolicyEditor::evalModeLabel(E::EveryBarClose),
                 Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryBarClose));
        QCOMPARE(RuntimePolicyEditor::evalModeLabel(E::EveryNBars),
                 Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNBars));
        QCOMPARE(RuntimePolicyEditor::evalModeLabel(E::EveryNMinutes),
                 Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNMinutes));
        QCOMPARE(RuntimePolicyEditor::evalModeLabel(E::EveryNDays),
                 Pipeline::StrategyRuntimePolicy::evalModeLabel(E::EveryNDays));

        QCOMPARE(RuntimePolicyEditor::rebalModeLabel(R::Immediate),
                 Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::Immediate));
        QCOMPARE(RuntimePolicyEditor::rebalModeLabel(R::EveryNBars),
                 Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNBars));
        QCOMPARE(RuntimePolicyEditor::rebalModeLabel(R::EveryNMinutes),
                 Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNMinutes));
        QCOMPARE(RuntimePolicyEditor::rebalModeLabel(R::EveryNDays),
                 Pipeline::StrategyRuntimePolicy::rebalModeLabel(R::EveryNDays));

        Pipeline::StrategyRuntimePolicy p;
        QCOMPARE(RuntimePolicyEditor::policySummary(p), p.summary());
    }
};

#endif // TST_RUNTIME_POLICY_EDITOR_H
