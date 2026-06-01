#ifndef STRATEGYLIFECYCLESTATE_H
#define STRATEGYLIFECYCLESTATE_H

#include <QJsonObject>
#include <QString>

namespace StrategyLifecycle {

enum class ManualLifecycle {
    Draft,
    Testing,
    Retired
};

enum class DerivedState {
    Draft,
    Testing,
    Ready,
    Live,
    Retired
};

struct Summary {
    ManualLifecycle lifecycle = ManualLifecycle::Draft;
    DerivedState state = DerivedState::Draft;
    int publishedVersionCount = 0;
    bool liveDeploymentActive = false;
    bool archived = false;
};

ManualLifecycle manualFromStorage(const QString& value);
QString manualToStorage(ManualLifecycle lifecycle);
QString manualLabel(ManualLifecycle lifecycle);
bool isAllowedManualStorageValue(const QString& value);

DerivedState deriveState(ManualLifecycle lifecycle,
                         bool archived,
                         int publishedVersionCount,
                         bool liveDeploymentActive);
QString stateKey(DerivedState state);
QString stateLabel(DerivedState state);
QString stateColorHex(DerivedState state);

Summary summarize(const QString& lifecycleState,
                  bool archived,
                  int publishedVersionCount,
                  bool liveDeploymentActive);
QJsonObject toJson(const Summary& summary);

} // namespace StrategyLifecycle

#endif // STRATEGYLIFECYCLESTATE_H
