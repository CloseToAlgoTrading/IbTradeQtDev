#ifndef CMODELSTATE_H
#define CMODELSTATE_H

#include <QString>

class CBaseModel;  // Forward declaration
enum class e_modelStateEvent;

// Internal state machine states (used by concrete state classes).
enum class e_modelState {
    MS_Init,
    MS_Init2,
    MS_Ready,
    MS_Running,
};

// UI-facing display state (derived from internal state + runtime info).
// This is NOT the internal state machine. See Design Decision D1.
// No business logic may depend on this enum.
enum class DisplayState {
    Disabled,       // user toggled off
    Idle,           // configured but not started
    Initializing,   // loading/connecting (maps from MS_Init, MS_Init2)
    Ready,          // warmup complete (maps from MS_Ready)
    Running,        // actively processing (maps from MS_Running)
    Paused,         // user-paused
    Warning,        // running but with issues
    Error,          // failed
    Disconnected    // lost connection (account-level)
};

class CModelState {
public:
    virtual ~CModelState () {}
    virtual void enterState(CBaseModel* model) = 0;
    virtual void handleEvent(CBaseModel* model, const e_modelStateEvent& event) = 0;
    virtual void exitState(CBaseModel* model) = 0;
    virtual e_modelState getStateID() const = 0;
};


#endif // CMODELSTATE_H
