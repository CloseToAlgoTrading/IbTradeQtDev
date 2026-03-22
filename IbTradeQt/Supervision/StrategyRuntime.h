#ifndef SUPERVISION_STRATEGYRUNTIME_H
#define SUPERVISION_STRATEGYRUNTIME_H

#include <QObject>
#include <QThread>
#include <QElapsedTimer>
#include <QDateTime>
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
        QObject* parent = nullptr);

    explicit StrategyRuntime(
        const QString& name,
        Pipeline::PipelineDefinition definition,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo,
        QObject* parent = nullptr);

    ~StrategyRuntime() override;

    void connectToMarketData(IBComm::MarketDataRouter* router);

    template<typename RouterT>
    void connectToMockRouter(RouterT* mockRouter) {
        m_runner->connectMarketDataFeed(mockRouter);
        m_runner->connectTickByTickFeed(mockRouter);
    }

    void start();
    void stop();
    void markCrashed(const QString& error);
    void heartbeat();

    QString name() const;
    bool isRunning() const;
    bool isHealthy() const;
    bool isCrashed() const;
    qint64 uptimeMs() const;
    QDateTime lastHeartbeat() const;
    QString lastError() const;
    int pipelineRunCount() const;
    int restartCount() const;
    void incrementRestartCount();

    Pipeline::StrategyPipelineRunner* runner();
    const Pipeline::BlockGraph& graph() const;
    const Pipeline::StrategyRuntimePolicy& runtimePolicy() const;

signals:
    void started(const QString& name);
    void stopped(const QString& name);
    void crashed(const QString& name, const QString& error);

private slots:
    void onThreadFinished();

private:
    void initThread();

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
