#include "StrategyLifecycleState.h"

#include <QtGlobal>

namespace StrategyLifecycle {

ManualLifecycle manualFromStorage(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("testing"))
        return ManualLifecycle::Testing;
    if (normalized == QStringLiteral("ready"))
        return ManualLifecycle::Ready;
    if (normalized == QStringLiteral("retired") || normalized == QStringLiteral("archived"))
        return ManualLifecycle::Retired;

    // Historical "active" did not mean publish-ready. Treat it as draft.
    return ManualLifecycle::Draft;
}

QString manualToStorage(ManualLifecycle lifecycle)
{
    switch (lifecycle) {
    case ManualLifecycle::Testing: return QStringLiteral("testing");
    case ManualLifecycle::Ready:   return QStringLiteral("ready");
    case ManualLifecycle::Retired: return QStringLiteral("retired");
    case ManualLifecycle::Draft:
    default:                       return QStringLiteral("draft");
    }
}

QString manualLabel(ManualLifecycle lifecycle)
{
    switch (lifecycle) {
    case ManualLifecycle::Testing: return QStringLiteral("Testing");
    case ManualLifecycle::Ready:   return QStringLiteral("Ready");
    case ManualLifecycle::Retired: return QStringLiteral("Retired");
    case ManualLifecycle::Draft:
    default:                       return QStringLiteral("Draft");
    }
}

bool isAllowedManualStorageValue(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("draft")
        || normalized == QStringLiteral("testing")
        || normalized == QStringLiteral("ready")
        || normalized == QStringLiteral("retired");
}

DerivedState deriveState(ManualLifecycle lifecycle,
                         bool archived,
                         int publishedVersionCount,
                         bool liveDeploymentActive)
{
    Q_UNUSED(publishedVersionCount);

    if (archived || lifecycle == ManualLifecycle::Retired)
        return DerivedState::Retired;
    if (liveDeploymentActive)
        return DerivedState::Live;
    if (lifecycle == ManualLifecycle::Ready)
        return DerivedState::Ready;
    if (lifecycle == ManualLifecycle::Testing)
        return DerivedState::Testing;
    return DerivedState::Draft;
}

QString stateKey(DerivedState state)
{
    switch (state) {
    case DerivedState::Testing: return QStringLiteral("testing");
    case DerivedState::Ready:   return QStringLiteral("ready");
    case DerivedState::Live:    return QStringLiteral("live");
    case DerivedState::Retired: return QStringLiteral("retired");
    case DerivedState::Draft:
    default:                    return QStringLiteral("draft");
    }
}

QString stateLabel(DerivedState state)
{
    switch (state) {
    case DerivedState::Testing: return QStringLiteral("Testing");
    case DerivedState::Ready:   return QStringLiteral("Ready");
    case DerivedState::Live:    return QStringLiteral("Live");
    case DerivedState::Retired: return QStringLiteral("Retired");
    case DerivedState::Draft:
    default:                    return QStringLiteral("Draft");
    }
}

QString stateColorHex(DerivedState state)
{
    switch (state) {
    case DerivedState::Live:    return QStringLiteral("#4caf50");
    case DerivedState::Ready:   return QStringLiteral("#58a6ff");
    case DerivedState::Testing: return QStringLiteral("#d4a04a");
    case DerivedState::Retired: return QStringLiteral("#888888");
    case DerivedState::Draft:
    default:                    return QStringLiteral("#aaaaaa");
    }
}

Summary summarize(const QString& lifecycleState,
                  bool archived,
                  int publishedVersionCount,
                  bool liveDeploymentActive)
{
    Summary summary;
    summary.lifecycle = manualFromStorage(lifecycleState);
    summary.publishedVersionCount = qMax(0, publishedVersionCount);
    summary.liveDeploymentActive = liveDeploymentActive;
    summary.archived = archived;
    summary.state = deriveState(summary.lifecycle,
                                summary.archived,
                                summary.publishedVersionCount,
                                summary.liveDeploymentActive);
    return summary;
}

QJsonObject toJson(const Summary& summary)
{
    QJsonObject obj;
    obj[QStringLiteral("lifecycle")] = manualToStorage(summary.lifecycle);
    obj[QStringLiteral("lifecycleLabel")] = manualLabel(summary.lifecycle);
    obj[QStringLiteral("state")] = stateKey(summary.state);
    obj[QStringLiteral("stateLabel")] = stateLabel(summary.state);
    obj[QStringLiteral("stateColor")] = stateColorHex(summary.state);
    obj[QStringLiteral("publishedVersionCount")] = summary.publishedVersionCount;
    obj[QStringLiteral("liveDeploymentActive")] = summary.liveDeploymentActive;
    obj[QStringLiteral("archived")] = summary.archived;
    return obj;
}

} // namespace StrategyLifecycle
