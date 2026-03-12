#ifndef TST_BLOCK_INTERFACES_H
#define TST_BLOCK_INTERFACES_H

#include <QtTest>
#include <QSignalSpy>
#include "Pipeline/IAlphaBlock.h"
#include "Pipeline/ISelectionBlock.h"
#include "Pipeline/IRebalanceBlock.h"
#include "Pipeline/IRiskBlock.h"
#include "Pipeline/IExecutionBlock.h"

// --- Mock implementations to verify interface contracts ---

class MockAlphaBlock : public Pipeline::IAlphaBlock
{
    Q_OBJECT
public:
    using IAlphaBlock::IAlphaBlock;

    QString id() const override { return "mock-alpha"; }
    QString name() const override { return "Mock Alpha"; }
    QString description() const override { return "Test mock"; }
    QJsonObject config() const override { return m_config; }
    void setConfig(const QJsonObject& config) override { m_config = config; }
    void initialize() override { m_initialized = true; }
    void shutdown() override { m_initialized = false; }

    void onTick(const IBComm::MarketTick& tick) override {
        m_lastTick = tick;
        m_tickCount++;
        if (tick.mid() > m_threshold) {
            Pipeline::Signal s;
            s.symbol = tick.symbol;
            s.direction = Pipeline::Signal::Buy;
            s.confidence = 0.8;
            s.alphaBlockId = id();
            emit signalGenerated(s);
        }
    }

    int m_tickCount = 0;
    double m_threshold = 0.0;
    bool m_initialized = false;
    IBComm::MarketTick m_lastTick;
    QJsonObject m_config;
};

class MockSelectionBlock : public Pipeline::ISelectionBlock
{
    Q_OBJECT
public:
    using ISelectionBlock::ISelectionBlock;

    QString id() const override { return "mock-selection"; }
    QString name() const override { return "Mock Selection"; }
    QString description() const override { return "Selects first N symbols"; }
    QJsonObject config() const override { return m_config; }
    void setConfig(const QJsonObject& config) override { m_config = config; }
    void initialize() override {}
    void shutdown() override {}

    QVector<QString> select(const QVector<QString>& universe) override {
        return universe.mid(0, qMin(m_maxSymbols, universe.size()));
    }

    int m_maxSymbols = 2;
    QJsonObject m_config;
};

class MockRebalanceBlock : public Pipeline::IRebalanceBlock
{
    Q_OBJECT
public:
    using IRebalanceBlock::IRebalanceBlock;

    QString id() const override { return "mock-rebalance"; }
    QString name() const override { return "Mock Rebalance"; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    QVector<Pipeline::TargetPosition> rebalance(
        const QVector<Pipeline::Signal>& inputSignals,
        const QMap<QString, double>& currentPositions) override
    {
        QVector<Pipeline::TargetPosition> targets;
        for (const auto& sig : inputSignals) {
            Pipeline::TargetPosition tp;
            tp.symbol = sig.symbol;
            tp.currentQuantity = currentPositions.value(sig.symbol, 0.0);
            tp.targetQuantity = (sig.direction == Pipeline::Signal::Buy) ? 100.0 :
                                (sig.direction == Pipeline::Signal::Sell) ? -100.0 : 0.0;
            tp.reason = sig.alphaBlockId;
            tp.correlationId = sig.correlationId;
            tp.timestamp = sig.timestamp;
            targets.append(tp);
        }
        return targets;
    }
};

class MockRiskBlock : public Pipeline::IRiskBlock
{
    Q_OBJECT
public:
    using IRiskBlock::IRiskBlock;

    QString id() const override { return "mock-risk"; }
    QString name() const override { return "Mock Risk"; }
    Pipeline::Scope scope() const override { return m_scope; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& target,
        const QVector<Pipeline::TargetPosition>&) override
    {
        m_evalCount++;
        if (std::abs(target.targetQuantity) > m_maxPosition) {
            Pipeline::RiskDecision d;
            d.action = Pipeline::RiskDecision::Action::Reject;
            d.reason = "Exceeds max position";
            d.blockId = id();
            return d;
        }
        Pipeline::RiskDecision d;
        d.action = Pipeline::RiskDecision::Action::Approve;
        d.reason = "Within limits";
        d.blockId = id();
        return d;
    }

    int m_evalCount = 0;
    double m_maxPosition = 500.0;
    Pipeline::Scope m_scope = Pipeline::Scope::Strategy;
};

class MockExecutionBlock : public Pipeline::IExecutionBlock
{
    Q_OBJECT
public:
    using IExecutionBlock::IExecutionBlock;

    QString id() const override { return "mock-execution"; }
    QString name() const override { return "Mock Execution"; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override {
        m_executedIntents = intents;
        for (const auto& intent : intents) {
            emit orderPlaced(intent.symbol, "ORD-" + intent.symbol);
        }
    }

    QVector<Pipeline::ExecutionIntent> m_executedIntents;
};


class TestBlockInterfaces : public QObject
{
    Q_OBJECT

private slots:
    void alphaBlockEmitsSignal()
    {
        MockAlphaBlock alpha;
        alpha.m_threshold = 100.0;
        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        IBComm::MarketTick tick;
        tick.symbol = "AAPL";
        tick.bid = 149.0;
        tick.ask = 151.0;
        alpha.onTick(tick);

        QCOMPARE(spy.count(), 1);
        auto signal = spy.at(0).at(0).value<Pipeline::Signal>();
        QCOMPARE(signal.symbol, QString("AAPL"));
        QCOMPARE(signal.direction, Pipeline::Signal::Buy);
    }

    void alphaBlockNoSignalBelowThreshold()
    {
        MockAlphaBlock alpha;
        alpha.m_threshold = 200.0;
        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        IBComm::MarketTick tick;
        tick.symbol = "AAPL";
        tick.bid = 149.0;
        tick.ask = 151.0;
        alpha.onTick(tick);

        QCOMPARE(spy.count(), 0);
    }

    void alphaBlockInitializeShutdown()
    {
        MockAlphaBlock alpha;
        QVERIFY(!alpha.m_initialized);
        alpha.initialize();
        QVERIFY(alpha.m_initialized);
        alpha.shutdown();
        QVERIFY(!alpha.m_initialized);
    }

    void alphaBlockConfig()
    {
        MockAlphaBlock alpha;
        QJsonObject cfg;
        cfg["lookback"] = 20;
        cfg["threshold"] = 0.5;
        alpha.setConfig(cfg);

        QCOMPARE(alpha.config()["lookback"].toInt(), 20);
        QCOMPARE(alpha.config()["threshold"].toDouble(), 0.5);
    }

    void selectionBlockSelectsSubset()
    {
        MockSelectionBlock selection;
        selection.m_maxSymbols = 2;

        QVector<QString> universe = {"AAPL", "MSFT", "GOOG", "TSLA"};
        auto result = selection.select(universe);

        QCOMPARE(result.size(), 2);
        QCOMPARE(result[0], QString("AAPL"));
        QCOMPARE(result[1], QString("MSFT"));
    }

    void selectionBlockEmptyUniverse()
    {
        MockSelectionBlock selection;
        auto result = selection.select({});
        QVERIFY(result.isEmpty());
    }

    void rebalanceBlockConvertsSignalsToPositions()
    {
        MockRebalanceBlock rebalance;
        QVector<Pipeline::Signal> sigList;

        Pipeline::Signal s;
        s.symbol = "AAPL";
        s.direction = Pipeline::Signal::Buy;
        s.confidence = 0.8;
        s.correlationId = "test-1";
        sigList.append(s);

        QMap<QString, double> positions;
        positions["AAPL"] = 50.0;

        auto targets = rebalance.rebalance(sigList, positions);
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets[0].symbol, QString("AAPL"));
        QCOMPARE(targets[0].targetQuantity, 100.0);
        QCOMPARE(targets[0].currentQuantity, 50.0);
    }

    void riskBlockApprovesWithinLimits()
    {
        MockRiskBlock risk;
        risk.m_maxPosition = 500.0;

        Pipeline::TargetPosition tp;
        tp.symbol = "AAPL";
        tp.targetQuantity = 100.0;

        auto decision = risk.evaluate(tp, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Approve);
        QCOMPARE(risk.m_evalCount, 1);
    }

    void riskBlockRejectsOverLimit()
    {
        MockRiskBlock risk;
        risk.m_maxPosition = 50.0;

        Pipeline::TargetPosition tp;
        tp.symbol = "AAPL";
        tp.targetQuantity = 100.0;

        auto decision = risk.evaluate(tp, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Reject);
        QVERIFY(!decision.reason.isEmpty());
    }

    void riskBlockScope()
    {
        MockRiskBlock stratRisk;
        stratRisk.m_scope = Pipeline::Scope::Strategy;
        QCOMPARE(stratRisk.scope(), Pipeline::Scope::Strategy);

        MockRiskBlock portfolioRisk;
        portfolioRisk.m_scope = Pipeline::Scope::Portfolio;
        QCOMPARE(portfolioRisk.scope(), Pipeline::Scope::Portfolio);

        MockRiskBlock accountRisk;
        accountRisk.m_scope = Pipeline::Scope::Account;
        QCOMPARE(accountRisk.scope(), Pipeline::Scope::Account);
    }

    void executionBlockExecutesIntents()
    {
        MockExecutionBlock exec;
        QSignalSpy spy(&exec, &Pipeline::IExecutionBlock::orderPlaced);

        QVector<Pipeline::ExecutionIntent> intents;
        Pipeline::ExecutionIntent ei;
        ei.symbol = "AAPL";
        ei.quantity = 100.0;
        ei.orderType = Pipeline::ExecutionIntent::Market;
        intents.append(ei);

        exec.execute(intents);

        QCOMPARE(exec.m_executedIntents.size(), 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("AAPL"));
    }

    void executionBlockMultipleIntents()
    {
        MockExecutionBlock exec;
        QSignalSpy spy(&exec, &Pipeline::IExecutionBlock::orderPlaced);

        QVector<Pipeline::ExecutionIntent> intents;
        Pipeline::ExecutionIntent ei1;
        ei1.symbol = "AAPL";
        ei1.quantity = 100.0;
        intents.append(ei1);

        Pipeline::ExecutionIntent ei2;
        ei2.symbol = "MSFT";
        ei2.quantity = -50.0;
        intents.append(ei2);

        exec.execute(intents);

        QCOMPARE(exec.m_executedIntents.size(), 2);
        QCOMPARE(spy.count(), 2);
    }

    void alphaBlockCountsTicks()
    {
        MockAlphaBlock alpha;
        alpha.m_threshold = 1000.0;

        IBComm::MarketTick tick;
        tick.symbol = "AAPL";
        tick.bid = 149.0;
        tick.ask = 151.0;

        alpha.onTick(tick);
        alpha.onTick(tick);
        alpha.onTick(tick);

        QCOMPARE(alpha.m_tickCount, 3);
    }

    void riskDecisionModifyAction()
    {
        Pipeline::RiskDecision d;
        d.action = Pipeline::RiskDecision::Action::Modify;
        d.reason = "Position too large";
        d.modifiedQuantity = -20.0;
        d.blockId = "max-pos-risk";

        QCOMPARE(d.action, Pipeline::RiskDecision::Action::Modify);
        QVERIFY(d.modifiedQuantity.has_value());
        QCOMPARE(*d.modifiedQuantity, -20.0);
    }
};

#endif // TST_BLOCK_INTERFACES_H
