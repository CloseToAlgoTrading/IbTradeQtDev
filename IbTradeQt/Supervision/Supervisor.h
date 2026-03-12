#ifndef SUPERVISION_SUPERVISOR_H
#define SUPERVISION_SUPERVISOR_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QDebug>
#include <functional>
#include "StrategyRuntime.h"

namespace Supervision {

enum class RestartPolicy {
    Never,
    Always,
    OnFailure
};

using RuntimeFactory = std::function<StrategyRuntime*()>;

class Supervisor : public QObject {
    Q_OBJECT

public:
    explicit Supervisor(QObject* parent = nullptr)
        : QObject(parent)
        , m_healthCheckTimer(new QTimer(this))
    {
        connect(m_healthCheckTimer, &QTimer::timeout,
                this, &Supervisor::checkHealth);
    }

    ~Supervisor() override {
        stopAll();
    }

    void addStrategy(const QString& name,
                     RuntimeFactory factory,
                     RestartPolicy policy = RestartPolicy::OnFailure)
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

    void removeStrategy(const QString& name) {
        auto it = m_runtimes.find(name);
        if (it == m_runtimes.end()) return;

        it->runtime->stop();
        delete it->runtime;
        m_runtimes.erase(it);
        emit strategyRemoved(name);
    }

    void startMonitoring(int intervalMs = 10000) {
        m_healthCheckTimer->start(intervalMs);
    }

    void stopMonitoring() {
        m_healthCheckTimer->stop();
    }

    void stopAll() {
        m_healthCheckTimer->stop();
        for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
            if (it->runtime) {
                it->runtime->stop();
            }
        }
    }

    int strategyCount() const { return m_runtimes.size(); }
    int maxRestartAttempts() const { return m_maxRestartAttempts; }
    void setMaxRestartAttempts(int n) { m_maxRestartAttempts = n; }

    bool isHealthy(const QString& name) const {
        auto it = m_runtimes.find(name);
        if (it == m_runtimes.end()) return false;
        return it->runtime->isHealthy();
    }

    bool isCrashed(const QString& name) const {
        auto it = m_runtimes.find(name);
        if (it == m_runtimes.end()) return false;
        return it->runtime->isCrashed();
    }

    int restartCount(const QString& name) const {
        auto it = m_runtimes.find(name);
        if (it == m_runtimes.end()) return -1;
        return it->restartCount;
    }

    StrategyRuntime* runtime(const QString& name) {
        auto it = m_runtimes.find(name);
        if (it == m_runtimes.end()) return nullptr;
        return it->runtime;
    }

    QStringList strategyNames() const {
        QStringList names;
        for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
            names.append(it.key());
        }
        return names;
    }

public slots:
    void checkHealth() {
        for (auto it = m_runtimes.begin(); it != m_runtimes.end(); ++it) {
            const QString& name = it.key();
            SupervisedRuntime& supervised = it.value();

            if (supervised.runtime->isCrashed()) {
                handleCrash(name, supervised);
            }
        }
        emit healthCheckCompleted();
    }

signals:
    void strategyAdded(const QString& name);
    void strategyRemoved(const QString& name);
    void strategyRestarted(const QString& name, int attempt);
    void strategyGaveUp(const QString& name, int attempts);
    void healthCheckCompleted();

private:
    struct SupervisedRuntime {
        StrategyRuntime* runtime;
        RuntimeFactory factory;
        RestartPolicy policy;
        int restartCount;
    };

    void connectRuntimeSignals(const QString& name, StrategyRuntime* runtime) {
        connect(runtime, &StrategyRuntime::crashed,
                this, [this, name](const QString&, const QString& error) {
            qCritical() << "Supervisor: strategy crashed:" << name << error;
        });
    }

    void handleCrash(const QString& name, SupervisedRuntime& supervised) {
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

    QMap<QString, SupervisedRuntime> m_runtimes;
    QTimer* m_healthCheckTimer;
    int m_maxRestartAttempts = 5;
};

} // namespace Supervision

#endif // SUPERVISION_SUPERVISOR_H
