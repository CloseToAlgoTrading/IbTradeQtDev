#include "Supervisor.h"

#include <QDebug>

namespace Supervision {

Supervisor::Supervisor(QObject* parent)
    : QObject(parent)
    , m_healthCheckTimer(new QTimer(this))
{
    connect(m_healthCheckTimer, &QTimer::timeout,
            this, &Supervisor::checkHealth);
}

Supervisor::~Supervisor()
{
    stopAll();
}

void Supervisor::addStrategy(const QString& name,
                 RuntimeFactory factory,
                 RestartPolicy policy)
{
    if (m_runtimes.contains(name)) {
        qWarning() << "Supervisor: strategy already registered:" << name;
        return;
    }

    auto* runtime = factory();
    connectRuntimeSignals(name, runtime);

    m_runtimes[name] = SupervisedRuntime{
        runtime, factory, policy, 0
    };

    runtime->start();
    emit strategyAdded(name);
}

void Supervisor::removeStrategy(const QString& name)
{
    auto it = m_runtimes.find(name);
    if (it == m_runtimes.end()) return;

    it->runtime->stop();
    delete it->runtime;
    m_runtimes.erase(it);
    emit strategyRemoved(name);
}

void Supervisor::startMonitoring(int intervalMs)
{
    m_healthCheckTimer->start(intervalMs);
}

void Supervisor::stopMonitoring()
{
    m_healthCheckTimer->stop();
}

void Supervisor::stopAll()
{
    m_healthCheckTimer->stop();
    for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
        if (it->runtime) {
            it->runtime->stop();
        }
    }
}

int Supervisor::strategyCount() const { return m_runtimes.size(); }
int Supervisor::maxRestartAttempts() const { return m_maxRestartAttempts; }
void Supervisor::setMaxRestartAttempts(int n) { m_maxRestartAttempts = n; }

bool Supervisor::isHealthy(const QString& name) const
{
    auto it = m_runtimes.find(name);
    if (it == m_runtimes.end()) return false;
    return it->runtime->isHealthy();
}

bool Supervisor::isCrashed(const QString& name) const
{
    auto it = m_runtimes.find(name);
    if (it == m_runtimes.end()) return false;
    return it->runtime->isCrashed();
}

int Supervisor::restartCount(const QString& name) const
{
    auto it = m_runtimes.find(name);
    if (it == m_runtimes.end()) return -1;
    return it->restartCount;
}

StrategyRuntime* Supervisor::runtime(const QString& name)
{
    auto it = m_runtimes.find(name);
    if (it == m_runtimes.end()) return nullptr;
    return it->runtime;
}

QStringList Supervisor::strategyNames() const
{
    QStringList names;
    for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
        names.append(it.key());
    }
    return names;
}

void Supervisor::checkHealth()
{
    for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
        const QString& name = it.key();
        SupervisedRuntime& supervised = it.value();

        if (supervised.runtime->isCrashed()) {
            handleCrash(name, supervised);
        }
    }
    emit healthCheckCompleted();
}

void Supervisor::connectRuntimeSignals(const QString& name, StrategyRuntime* runtime)
{
    connect(runtime, &StrategyRuntime::crashed,
            this, [this, name](const QString&, const QString& error) {
        qCritical() << "Supervisor: strategy crashed:" << name << error;
    });
}

void Supervisor::handleCrash(const QString& name, SupervisedRuntime& supervised)
{
    if (supervised.policy == RestartPolicy::Never) return;

    if (supervised.restartCount >= m_maxRestartAttempts) {
        qCritical() << "Supervisor:" << name
                   << "exceeded max restart attempts (" << m_maxRestartAttempts
                   << "), giving up";
        supervised.policy = RestartPolicy::Never;
        emit strategyGaveUp(name, supervised.restartCount);
        return;
    }

    supervised.restartCount++;
    qInfo() << "Supervisor: restarting" << name
            << "attempt" << supervised.restartCount;

    supervised.runtime->stop();
    delete supervised.runtime;

    supervised.runtime = supervised.factory();
    connectRuntimeSignals(name, supervised.runtime);
    supervised.runtime->incrementRestartCount();
    supervised.runtime->start();

    emit strategyRestarted(name, supervised.restartCount);
}

} // namespace Supervision
