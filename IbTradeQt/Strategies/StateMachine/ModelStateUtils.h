#ifndef MODELSTATEUTILS_H
#define MODELSTATEUTILS_H

#include "cmodelstate.h"
#include <QColor>
#include <QString>

struct StateDisplayInfo {
    QString label;
    QColor  color;
    QString indicator;
};

namespace ModelStateUtils {

inline StateDisplayInfo stateDisplay(DisplayState s) {
    switch (s) {
    case DisplayState::Disabled:
        return {"Disabled",      QColor(102, 102, 102), QString::fromUtf8("\xE2\x97\x8B")};  // ○
    case DisplayState::Idle:
        return {"Idle",          QColor(136, 136, 136), QString::fromUtf8("\xE2\x96\xA0")};  // ■
    case DisplayState::Initializing:
        return {"Initializing",  QColor(80, 125, 188),  QString::fromUtf8("\xE2\x97\x8F")};  // ●
    case DisplayState::Ready:
        return {"Ready",         QColor(80, 125, 188),  QString::fromUtf8("\xE2\x97\x8F")};  // ●
    case DisplayState::Running:
        return {"Running",       QColor(76, 175, 80),   QString::fromUtf8("\xE2\x97\x8F")};  // ●
    case DisplayState::Paused:
        return {"Paused",        QColor(255, 152, 0),   QString::fromUtf8("\xE2\x96\xA0")};  // ■
    case DisplayState::Warning:
        return {"Warning",       QColor(255, 152, 0),   QString::fromUtf8("\xE2\x96\xB2")};  // ▲
    case DisplayState::Error:
        return {"Error",         QColor(229, 57, 53),   QString::fromUtf8("\xE2\x9C\x96")};  // ✖
    case DisplayState::Disconnected:
        return {"Disconnected",  QColor(229, 57, 53),   QString::fromUtf8("\xE2\x9C\x96")};  // ✖
    }
    return {"Unknown", QColor(102, 102, 102), "?"};
}

// Maps internal state machine state to display state (transitional adapter).
// Also considers activation flag and status strings from genericInfo.
inline DisplayState resolveFromInternal(e_modelState internalState,
                                        bool isActivated,
                                        const QString& statusString)
{
    if (!isActivated)
        return DisplayState::Disabled;

    if (statusString == QLatin1String("Disconnected"))
        return DisplayState::Disconnected;
    if (statusString == QLatin1String("Error"))
        return DisplayState::Error;
    if (statusString == QLatin1String("Warning"))
        return DisplayState::Warning;
    if (statusString == QLatin1String("Paused"))
        return DisplayState::Paused;

    switch (internalState) {
    case e_modelState::MS_Init:
    case e_modelState::MS_Init2:
        return DisplayState::Initializing;
    case e_modelState::MS_Ready:
        return DisplayState::Ready;
    case e_modelState::MS_Running:
        return DisplayState::Running;
    }

    return DisplayState::Idle;
}

} // namespace ModelStateUtils

#endif // MODELSTATEUTILS_H
