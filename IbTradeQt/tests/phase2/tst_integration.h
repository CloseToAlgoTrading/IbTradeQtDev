#ifndef TST_INTEGRATION_H
#define TST_INTEGRATION_H

#include <QtTest>
#include <QSignalSpy>
#include "Testing/IntegrationTestHarness.h"

// Simple mock alpha that emits Buy when price rises above a threshold
class TestMomentumAlpha : public Pipeline::IAlphaBlock
{
    Q_OBJECT
public:
    using IAlphaBlock::IAlphaBlock;

    QString id() const override { return "test-momentum"; }
    QString name() const override { return "Test Momentum"; }
    QString description() const override { return "Emits Buy if mid > threshold"; }
    QJsonObject config() const override { return m_config; }
    void setConfig(const QJsonObject& c) override {
        m_config = c;
        m_threshold = c.value("threshold").toDouble(151.0);
    }
    void initialize() override {}
    void shutdown() override {}

    void onTick(const IBComm::MarketTick& tick) override {
        m_lastMid = tick.mid();
        if (m_lastMid > m_threshold && !m_signalSent) {
            Pipeline::Signal sig;
            sig.symbol = tick.symbol;
            sig.direction = Pipeline::Signal::Buy;
            sig.confidence = 0.85;
            sig.alphaBlockId = id();
            sig.timestamp = tick.timestamp;
            sig.correlationId = QString("corr-%1").arg(++m_counter);
            emit signalGenerated(sig);
            m_signalSent = true;
        }
    }

    bool m_signalSent = false;
    double m_lastMid = 0.0;
    double m_threshold = 151.0;
    int m_counter = 0;
    QJsonObject m_config;
};

// Simple rebalance: 100 shares per Buy signal
class TestSimpleRebalance : public Pipeline::IRebalanceBlock
{
    Q_OBJECT
public:
    using IRebalanceBlock::IRebalanceBlock;

    QString id() const override { return "simple-rebalance"; }
    QString name() const override { return "Simple Rebalance"; }
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

// No-op risk block for testing
class TestNoOpRisk : public Pipeline::IRiskBlock
{
    Q_OBJECT
public:
    using IRiskBlock::IRiskBlock;

    QString id() const override { return "noop-risk"; }
    QString name() const override { return "NoOp Risk"; }
    Pipeline::Scope scope() const override { return Pipeline::Scope::Strategy; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition&,
        const QVector<Pipeline::TargetPosition>&,
        const QMap<QString, double>&) override
    {
        return {Pipeline::RiskDecision::Action::Approve, "No-op", {}, id()};
    }
};

// Execution block that records intents using mock adapter
class TestMockExecBlock : public Pipeline::IExecutionBlock
{
    Q_OBJECT
public:
    explicit TestMockExecBlock(MockExecutionAdapter* adapter, QObject* parent = nullptr)
        : IExecutionBlock(parent), m_adapter(adapter) {}

    QString id() const override { return "mock-exec"; }
    QString name() const override { return "Mock Execution"; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override {
        for (const auto& intent : intents) {
            auto result = m_adapter->placeOrder(intent);
            if (result.has_value()) {
                emit orderPlaced(intent.symbol, QString::number(result->orderId));
            } else {
                emit executionError(intent.symbol,
                    QString::fromStdString(result.error().message));
            }
        }
    }

private:
    MockExecutionAdapter* m_adapter;
};


// Risk block that rejects positions > maxQty
class TestStrictRisk : public Pipeline::IRiskBlock {
    Q_OBJECT
public:
    explicit TestStrictRisk(double maxQty = 50.0, QObject* parent = nullptr)
        : IRiskBlock(parent), m_maxQty(maxQty) {}

    QString id() const override { return "strict"; }
    QString name() const override { return "Strict Risk"; }
    Pipeline::Scope scope() const override { return Pipeline::Scope::Strategy; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& tp,
        const QVector<Pipeline::TargetPosition>&,
        const QMap<QString, double>&) override
    {
        if (std::abs(tp.targetQuantity) > m_maxQty) {
            return {Pipeline::RiskDecision::Action::Reject,
                    "Too large", {}, id()};
        }
        return {Pipeline::RiskDecision::Action::Approve,
                "OK", {}, id()};
    }

private:
    double m_maxQty;
};


class TestIntegration : public QObject
{
    Q_OBJECT

private slots:
    void alphaReceivesTicksFromMockRouter()
    {
        IntegrationTestHarness harness;
        TestMomentumAlpha alpha;
        harness.connectAlphaToRouter(&alpha);

        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        QSignalSpy spy(&alpha, &Pipeline::IAlphaBlock::signalGenerated);

        harness.router()->simulateTick("AAPL", 148.0, 148.5);
        QCOMPARE(spy.count(), 0);

        harness.router()->simulateTick("AAPL", 151.0, 152.0);
        QCOMPARE(spy.count(), 1);

        auto sig = spy.at(0).at(0).value<Pipeline::Signal>();
        QCOMPARE(sig.symbol, QString("AAPL"));
        QCOMPARE(sig.direction, Pipeline::Signal::Buy);
    }

    void rebalanceConvertsSignalToTarget()
    {
        TestSimpleRebalance rebalance;

        Pipeline::Signal sig;
        sig.symbol = "AAPL";
        sig.direction = Pipeline::Signal::Buy;
        sig.confidence = 0.85;
        sig.correlationId = "corr-1";

        QMap<QString, double> positions;
        positions["AAPL"] = 50.0;

        auto targets = rebalance.rebalance({sig}, positions);
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets[0].symbol, QString("AAPL"));
        QCOMPARE(targets[0].targetQuantity, 100.0);
        QCOMPARE(targets[0].currentQuantity, 50.0);
        QCOMPARE(targets[0].correlationId, QString("corr-1"));
    }

    void riskNoOpApprovesAll()
    {
        TestNoOpRisk risk;
        Pipeline::TargetPosition tp;
        tp.symbol = "AAPL";
        tp.targetQuantity = 1000.0;

        auto decision = risk.evaluate(tp, {}, {});
        QCOMPARE(decision.action, Pipeline::RiskDecision::Action::Approve);
    }

    void executionBlockPlacesOrders()
    {
        IntegrationTestHarness harness;
        TestMockExecBlock exec(harness.execution());

        QVector<Pipeline::ExecutionIntent> intents;
        Pipeline::ExecutionIntent ei;
        ei.symbol = "AAPL";
        ei.quantity = 100.0;
        ei.orderType = Pipeline::ExecutionIntent::Market;
        intents.append(ei);

        exec.execute(intents);
        QCOMPARE(harness.placedOrders().size(), 1);
        QCOMPARE(harness.placedOrders()[0].symbol, QString("AAPL"));
    }

    void endToEndMinimalPipeline()
    {
        IntegrationTestHarness harness;

        TestMomentumAlpha alpha;
        alpha.m_threshold = 151.0;
        harness.connectAlphaToRouter(&alpha);

        TestSimpleRebalance rebalance;
        TestNoOpRisk risk;
        TestMockExecBlock exec(harness.execution());

        // Wire alpha → rebalance → risk → execution manually
        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        connect(&alpha, &Pipeline::IAlphaBlock::signalGenerated,
                this, [&](const Pipeline::Signal& sig) {
            auto targets = rebalance.rebalance({sig}, {});
            QVector<Pipeline::ExecutionIntent> intents;
            for (const auto& tp : targets) {
                auto decision = risk.evaluate(tp, targets, {});
                if (decision.action == Pipeline::RiskDecision::Action::Approve) {
                    Pipeline::ExecutionIntent ei;
                    ei.symbol = tp.symbol;
                    ei.quantity = tp.deltaQuantity();
                    ei.orderType = Pipeline::ExecutionIntent::Market;
                    ei.correlationId = tp.correlationId;
                    ei.timestamp = tp.timestamp;
                    intents.append(ei);
                }
            }
            if (!intents.isEmpty()) {
                exec.execute(intents);
            }
        });

        // Simulate uptrend
        QDateTime base(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());
        harness.router()->simulateTick("AAPL", 148.0, 148.5, base);
        harness.router()->simulateTick("AAPL", 149.0, 149.5, base.addSecs(60));
        harness.router()->simulateTick("AAPL", 151.0, 152.0, base.addSecs(120));

        QCOMPARE(harness.placedOrders().size(), 1);
        QCOMPARE(harness.placedOrders()[0].symbol, QString("AAPL"));
        QVERIFY(harness.placedOrders()[0].quantity > 0);
    }

    void endToEndWithRiskRejection()
    {
        IntegrationTestHarness harness;

        TestMomentumAlpha alpha;
        alpha.m_threshold = 100.0;
        harness.connectAlphaToRouter(&alpha);

        TestSimpleRebalance rebalance;
        TestStrictRisk risk(50.0);
        TestMockExecBlock exec(harness.execution());

        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        connect(&alpha, &Pipeline::IAlphaBlock::signalGenerated,
                this, [&](const Pipeline::Signal& sig) {
            auto targets = rebalance.rebalance({sig}, {});
            QVector<Pipeline::ExecutionIntent> intents;
            for (const auto& tp : targets) {
                auto decision = risk.evaluate(tp, targets, {});
                if (decision.action == Pipeline::RiskDecision::Action::Approve) {
                    Pipeline::ExecutionIntent ei;
                    ei.symbol = tp.symbol;
                    ei.quantity = tp.deltaQuantity();
                    intents.append(ei);
                }
            }
            if (!intents.isEmpty()) exec.execute(intents);
        });

        // Alpha generates 100-share target → risk rejects (>50)
        QDateTime base(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());
        harness.router()->simulateTick("AAPL", 150.0, 152.0, base);

        QCOMPARE(harness.placedOrders().size(), 0);
    }

    void replayProducesSameResultsAsLive()
    {
        // Simulate "live" run
        IntegrationTestHarness harness1;
        TestMomentumAlpha alpha1;
        alpha1.m_threshold = 151.0;
        harness1.connectAlphaToRouter(&alpha1);

        qRegisterMetaType<Pipeline::Signal>("Pipeline::Signal");
        QVector<Pipeline::Signal> liveSignals;
        connect(&alpha1, &Pipeline::IAlphaBlock::signalGenerated,
                this, [&](const Pipeline::Signal& sig) {
            liveSignals.append(sig);
        });

        QDateTime base(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());
        QVector<IBComm::MarketTick> ticks;
        for (int i = 0; i < 5; ++i) {
            IBComm::MarketTick t;
            t.symbol = "AAPL";
            t.bid = 148.0 + i;
            t.ask = 148.5 + i;
            t.timestamp = base.addSecs(i * 60);
            ticks.append(t);
        }

        harness1.runWithTicks(ticks);

        // Now replay with same ticks
        IntegrationTestHarness harness2;
        TestMomentumAlpha alpha2;
        alpha2.m_threshold = 151.0;
        harness2.connectAlphaToRouter(&alpha2);

        QVector<Pipeline::Signal> replaySignals;
        connect(&alpha2, &Pipeline::IAlphaBlock::signalGenerated,
                this, [&](const Pipeline::Signal& sig) {
            replaySignals.append(sig);
        });

        harness2.runWithTicks(ticks);

        // Results must be identical
        QCOMPARE(liveSignals.size(), replaySignals.size());
        for (int i = 0; i < liveSignals.size(); ++i) {
            QCOMPARE(liveSignals[i].symbol, replaySignals[i].symbol);
            QCOMPARE(liveSignals[i].direction, replaySignals[i].direction);
            QCOMPARE(liveSignals[i].confidence, replaySignals[i].confidence);
        }
    }
};

#endif // TST_INTEGRATION_H
