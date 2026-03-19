#ifndef SUPERVISION_STRATEGYRUNTIME_H
#define SUPERVISION_STRATEGYRUNTIME_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <atomic>
#include "../Pipeline/StrategyPipelineRunner.h"
#include "../Pipeline/PipelineDefinition.h"
#include "../IBComm/MarketDataRouter.h"
#include "../Ports/IOrderExecutionPort.h"
#include "../Ports/IPositionRepositoryPort.h"

namespace Supervision {

class StrategyRuntime : public QObject {
    Q_OBJECT

public:
    explicit StrategyRuntime(
        const QString& name,
        Pipeline::BlockGraph graph,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        QObject* parent = nullptr)
        : QObject(parent)
        , m_name(name)
        , m_graph(std::move(graph))
        , m_executionPort(executionPort)
        , m_positionRepo(positionRepo)
    {
        initThread();
    }

    explicit StrategyRuntime(
        const QString& name,
        Pipeline::PipelineDefinition definition,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        QObject* parent = nullptr)
        : QObject(parent)
        , m_name(name)
        , m_graph(std::move(definition.graph))
        , m_runtimePolicy(definition.runtimePolicy)
        , m_executionPort(executionPort)
        , m_positionRepo(positionRepo)
    {
        initThread();
    }

    ~StrategyRuntime() override {
        stop();
        if (m_thread->isRunning()) {
            m_thread->quit();
            m_thread->wait(5000);
        }
        delete m_runner;
        delete m_thread;
    }

    void connectToMarketData(IBComm::MarketDataRouter* router) {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(router, &IBComm::MarketDataRouter::tick,
                    alpha, &Pipeline::IAlphaBlock::onTick,
                    Qt::QueuedConnection);
            connect(router, &IBComm::MarketDataRouter::tickByTickTrade,
                    alpha, &Pipeline::IAlphaBlock::onTickByTick,
                    Qt::QueuedConnection);
        }

        auto allRisks = m_graph.strategyLevel.risks
                      + m_graph.portfolioLevel.risks
                      + m_graph.accountLevel.risks;
        for (auto* risk : allRisks) {
            connect(router, &IBComm::MarketDataRouter::tick,
                    risk, [risk](const IBComm::MarketTick& t){ risk->onTick(t); },
                    Qt::QueuedConnection);
        }

        connect(router, &IBComm::MarketDataRouter::barClose,
                m_runner, &Pipeline::StrategyPipelineRunner::onBarClose,
                Qt::QueuedConnection);
    }

    template<typename RouterT>
    void connectToMockRouter(RouterT* mockRouter) {
        for (auto* alpha : m_graph.alphaBlocks) {
            connect(mockRouter, &RouterT::tick,
                    alpha, &Pipeline::IAlphaBlock::onTick,
                    Qt::DirectConnection);
            connect(mockRouter, &RouterT::barClose,
                    alpha, &Pipeline::IAlphaBlock::onBarClose,
                    Qt::DirectConnection);
        }

        auto allRisks = m_graph.strategyLevel.risks
                      + m_graph.portfolioLevel.risks
                      + m_graph.accountLevel.risks;
        for (auto* risk : allRisks) {
            connect(mockRouter, &RouterT::tick,
                    risk, [risk](const IBComm::MarketTick& t){ risk->onTick(t); },
                    Qt::DirectConnection);
        }

        connect(mockRouter, &RouterT::barClose,
                m_runner, &Pipeline::StrategyPipelineRunner::onBarClose,
                Qt::DirectConnection);
    }

    void start() {
        if (m_running) return;
        m_running = true;
        m_crashed = false;
        m_uptimeTimer.start();
        m_thread->start();
        m_lastHeartbeat = QDateTime::currentDateTime();
        emit started(m_name);
    }

    void stop() {
        if (!m_running) return;
        m_running = false;
        m_thread->quit();
        emit stopped(m_name);
    }

    void markCrashed(const QString& error) {
        m_crashed = true;
        m_running = false;
        m_lastError = error;
        m_thread->quit();
        emit crashed(m_name, error);
    }

    void heartbeat() {
        m_lastHeartbeat = QDateTime::currentDateTime();
        m_pipelineRunCount++;
    }

    // Accessors
    QString name() const { return m_name; }

    bool isRunning() const { return m_running && m_thread->isRunning(); }

    bool isHealthy() const {
        if (m_crashed) return false;
        if (!m_running) return true; // stopped is not unhealthy
        if (!m_thread->isRunning()) return false;
        return true;
    }

    bool isCrashed() const { return m_crashed; }

    qint64 uptimeMs() const {
        return m_running ? m_uptimeTimer.elapsed() : 0;
    }

    QDateTime lastHeartbeat() const { return m_lastHeartbeat; }
    QString lastError() const { return m_lastError; }
    int pipelineRunCount() const { return m_pipelineRunCount; }
    int restartCount() const { return m_restartCount; }
    void incrementRestartCount() { m_restartCount++; }

    Pipeline::StrategyPipelineRunner* runner() { return m_runner; }
    const Pipeline::BlockGraph& graph() const { return m_graph; }
    const Pipeline::StrategyRuntimePolicy& runtimePolicy() const { return m_runtimePolicy; }

signals:
    void started(const QString& name);
    void stopped(const QString& name);
    void crashed(const QString& name, const QString& error);

private slots:
    void onThreadFinished() {
        if (m_running && !m_crashed) {
            markCrashed("Thread terminated unexpectedly");
        }
    }

    void initThread() {
        m_thread = new QThread();
        m_thread->setObjectName("Strategy-" + m_name);

        m_runner = new Pipeline::StrategyPipelineRunner(
            m_graph, m_runtimePolicy, m_executionPort, m_positionRepo);
        m_runner->wireAlphaSignals();

        m_runner->moveToThread(m_thread);

        for (auto* alpha : m_graph.alphaBlocks) {
            alpha->moveToThread(m_thread);
        }

        connect(m_thread, &QThread::finished,
                this, &StrategyRuntime::onThreadFinished);
    }

private:
    QString m_name;
    Pipeline::BlockGraph m_graph;
    Pipeline::StrategyRuntimePolicy m_runtimePolicy;
    Ports::IOrderExecutionPort* m_executionPort;
    Ports::IPositionRepositoryPort* m_positionRepo;

    Pipeline::StrategyPipelineRunner* m_runner = nullptr;
    QThread* m_thread = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_crashed{false};
    std::atomic<int> m_pipelineRunCount{0};
    int m_restartCount = 0;

    QElapsedTimer m_uptimeTimer;
    QDateTime m_lastHeartbeat;
    QString m_lastError;
};

} // namespace Supervision

#endif // SUPERVISION_STRATEGYRUNTIME_H
