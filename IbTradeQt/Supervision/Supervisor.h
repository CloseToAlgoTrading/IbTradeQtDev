#ifndef SUPERVISION_SUPERVISOR_H
#define SUPERVISION_SUPERVISOR_H

#include <QObject>
#include <QTimer>
#include <QMap>
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
    explicit Supervisor(QObject* parent = nullptr);
    ~Supervisor() override;

    void addStrategy(const QString& name,
                     RuntimeFactory factory,
                     RestartPolicy policy = RestartPolicy::OnFailure);

    void removeStrategy(const QString& name);

    void startMonitoring(int intervalMs = 10000);
    void stopMonitoring();

    void stopAll();

    int strategyCount() const;
    int maxRestartAttempts() const;
    void setMaxRestartAttempts(int n);

    bool isHealthy(const QString& name) const;
    bool isCrashed(const QString& name) const;
    int restartCount(const QString& name) const;

    StrategyRuntime* runtime(const QString& name);
    QStringList strategyNames() const;

public slots:
    void checkHealth();

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

    void connectRuntimeSignals(const QString& name, StrategyRuntime* runtime);
    void handleCrash(const QString& name, SupervisedRuntime& supervised);

    QMap<QString, SupervisedRuntime> m_runtimes;
    QTimer* m_healthCheckTimer;
    int m_maxRestartAttempts = 5;
};

} // namespace Supervision

#endif // SUPERVISION_SUPERVISOR_H
