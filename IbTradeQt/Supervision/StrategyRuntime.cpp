#include "StrategyRuntime.h"

#include <QDebug>
#include <QThread>

namespace Supervision {

StrategyRuntime::StrategyRuntime(
    const QString& name,
    Pipeline::BlockGraph graph,
    Ports::IOrderExecutionPort* executionPort,
    Ports::IPositionRepositoryPort* positionRepo,
    QObject* parent)
    : QObject(parent)
    , m_name(name)
    , m_graph(std::move(graph))
    , m_executionPort(executionPort)
    , m_positionRepo(positionRepo)
{
    initThread();
}

StrategyRuntime::StrategyRuntime(
    const QString& name,
    Pipeline::PipelineDefinition definition,
    Ports::IOrderExecutionPort* executionPort,
    Ports::IPositionRepositoryPort* positionRepo,
    QObject* parent)
    : QObject(parent)
    , m_name(name)
    , m_graph(std::move(definition.graph))
    , m_runtimePolicy(definition.runtimePolicy)
    , m_executionPort(executionPort)
    , m_positionRepo(positionRepo)
{
    initThread();
}

StrategyRuntime::~StrategyRuntime()
{
    stop();
    if (m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(5000);
    }
    delete m_runner;
    delete m_thread;
}

void StrategyRuntime::connectToMarketData(IBComm::MarketDataRouter* router)
{
    m_runner->connectToMarketData(router);
}

void StrategyRuntime::start()
{
    if (m_running) return;
    m_running = true;
    m_crashed = false;
    m_uptimeTimer.start();
    m_thread->start();
    m_lastHeartbeat = QDateTime::currentDateTime();
    emit started(m_name);
}

void StrategyRuntime::stop()
{
    if (!m_running) return;
    m_running = false;
    if (m_thread && m_thread->isRunning())
        QMetaObject::invokeMethod(m_runner, "ping", Qt::BlockingQueuedConnection);
    m_thread->quit();
    emit stopped(m_name);
}

void StrategyRuntime::markCrashed(const QString& error)
{
    m_crashed = true;
    m_running = false;
    m_lastError = error;
    m_thread->quit();
    emit crashed(m_name, error);
}

void StrategyRuntime::heartbeat()
{
    m_lastHeartbeat = QDateTime::currentDateTime();
    m_pipelineRunCount++;
}

QString StrategyRuntime::name() const { return m_name; }

bool StrategyRuntime::isRunning() const
{
    return m_running && m_thread->isRunning();
}

bool StrategyRuntime::isHealthy() const
{
    if (m_crashed) return false;
    if (!m_running) return true;
    if (!m_thread->isRunning()) return false;
    return true;
}

bool StrategyRuntime::isCrashed() const { return m_crashed; }

qint64 StrategyRuntime::uptimeMs() const
{
    return m_running ? m_uptimeTimer.elapsed() : 0;
}

QDateTime StrategyRuntime::lastHeartbeat() const { return m_lastHeartbeat; }
QString StrategyRuntime::lastError() const { return m_lastError; }
int StrategyRuntime::pipelineRunCount() const { return m_pipelineRunCount; }
int StrategyRuntime::restartCount() const { return m_restartCount; }
void StrategyRuntime::incrementRestartCount() { m_restartCount++; }

Pipeline::StrategyPipelineRunner* StrategyRuntime::runner() { return m_runner; }
const Pipeline::BlockGraph& StrategyRuntime::graph() const { return m_graph; }
const Pipeline::StrategyRuntimePolicy& StrategyRuntime::runtimePolicy() const { return m_runtimePolicy; }

void StrategyRuntime::onThreadFinished()
{
    if (m_running && !m_crashed) {
        markCrashed(QStringLiteral("Thread terminated unexpectedly"));
    }
}

void StrategyRuntime::initThread()
{
    m_thread = new QThread();
    m_thread->setObjectName(QStringLiteral("Strategy-") + m_name);

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

} // namespace Supervision
