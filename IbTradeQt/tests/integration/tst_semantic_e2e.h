#ifndef TST_SEMANTIC_E2E_H
#define TST_SEMANTIC_E2E_H

#include <QtTest>
#include <QSignalSpy>
#include <QTimer>
#include "Pipeline/StrategyPipelineRunner.h"
#include "Blocks/MarketOrderExecutionBlock.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Strategies/Generic/UnifiedModelData.h"

/// Sync alpha: fills semantic rows for symbols from selection (AAPL → long 10 sh).
class SemanticE2EAlpha : public Pipeline::IAlphaBlock {
    Q_OBJECT
public:
    explicit SemanticE2EAlpha(QObject* parent = nullptr)
        : Pipeline::IAlphaBlock(parent)
    {}

    QString id() const override { return QStringLiteral("semantic-e2e-alpha"); }
    QString name() const override { return QStringLiteral("Semantic E2E Alpha"); }
    QString description() const override { return {}; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}
    void initialize() override {}
    void shutdown() override {}
    void onTick(const Pipeline::MarketTick&) override {}

    Pipeline::ModelDataList processSemantic(const Pipeline::ModelDataList& in,
                                           const QString& correlationId) override
    {
        Q_UNUSED(correlationId);
        if (!in)
            return createDataList();
        Pipeline::ModelDataList out = createDataList();
        for (const auto& row : *in) {
            UnifiedModelData u = row;
            if (u.symbol == QStringLiteral("AAPL")) {
                u.direction = DIRECTION_UP;
                u.amount = 10.0;
                u.probability = 1.0;
            }
            out->append(u);
        }
        return out;
    }
};

/// Async alpha: defers semantic output to the next event-loop turn (semanticReady).
class SemanticE2EAsyncAlpha : public Pipeline::IAlphaBlock {
    Q_OBJECT
public:
    explicit SemanticE2EAsyncAlpha(QObject* parent = nullptr)
        : Pipeline::IAlphaBlock(parent)
    {}

    QString id() const override { return QStringLiteral("semantic-e2e-async-alpha"); }
    QString name() const override { return QStringLiteral("Semantic E2E Async Alpha"); }
    QString description() const override { return {}; }
    QJsonObject config() const override { return {}; }
    void setConfig(const QJsonObject&) override {}
    void initialize() override {}
    void shutdown() override {}
    void onTick(const Pipeline::MarketTick&) override {}

    bool semanticCompletionIsAsync() const override { return true; }

    Pipeline::ModelDataList processSemantic(const Pipeline::ModelDataList& in,
                                           const QString& correlationId) override
    {
        m_pendingCorr = correlationId;
        m_pendingIn = in;
        QTimer::singleShot(0, this, [this]() {
            Pipeline::ModelDataList out = createDataList();
            if (m_pendingIn) {
                for (const auto& row : *m_pendingIn) {
                    UnifiedModelData u = row;
                    if (u.symbol == QStringLiteral("AAPL")) {
                        u.direction = DIRECTION_UP;
                        u.amount = 7.0;
                        u.probability = 1.0;
                    }
                    out->append(u);
                }
            }
            emit semanticReady(out, m_pendingCorr);
        });
        return in;
    }

private:
    QString m_pendingCorr;
    Pipeline::ModelDataList m_pendingIn;
};

class TestSemanticE2E : public QObject {
    Q_OBJECT
private slots:
    void semantic_model_rebalance_places_order()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        SemanticE2EAlpha alpha;
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;
        graph.config.insert(QStringLiteral("semanticPipeline"), true);
        graph.config.insert(QStringLiteral("semanticModelRebalance"), true);

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();
        runner.setUniverse({QStringLiteral("AAPL")});

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);
        runner.runPipeline();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(mockExec.placedOrders().size(), 1);
        QCOMPARE(mockExec.placedOrders()[0].symbol, QStringLiteral("AAPL"));
        QVERIFY(qAbs(mockExec.placedOrders()[0].quantity - 10.0) < 1e-6);
    }

    void async_semantic_alpha_finishes_pipeline()
    {
        MockExecutionAdapter mockExec;
        MockPositionRepository mockRepo;

        SemanticE2EAsyncAlpha alpha;
        Blocks::SimpleRebalanceBlock rebalance;
        Blocks::MarketOrderExecutionBlock execution;
        execution.setExecutionPort(&mockExec);

        Pipeline::BlockGraph graph;
        graph.alphaBlocks.append(&alpha);
        graph.strategyLevel.rebalance = &rebalance;
        graph.executionBlock = &execution;
        graph.config.insert(QStringLiteral("semanticPipeline"), true);
        graph.config.insert(QStringLiteral("semanticModelRebalance"), true);

        Pipeline::StrategyPipelineRunner runner(graph, &mockExec, &mockRepo);
        runner.wireAlphaSignals();
        runner.setUniverse({QStringLiteral("AAPL")});

        QSignalSpy spy(&runner, &Pipeline::StrategyPipelineRunner::pipelineCompleted);
        runner.runPipeline();
        QVERIFY(spy.wait(3000));
        QCOMPARE(mockExec.placedOrders().size(), 1);
        QCOMPARE(mockExec.placedOrders()[0].symbol, QStringLiteral("AAPL"));
        QVERIFY(qAbs(mockExec.placedOrders()[0].quantity - 7.0) < 1e-6);
    }
};

#endif // TST_SEMANTIC_E2E_H
