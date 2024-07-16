#ifndef CMODELSTATE_H
#define CMODELSTATE_H

#include <QString>

class CBaseModel;  // Forward declaration
enum class e_modelStateEvent;

enum class e_modelState {
    MS_Init,
    MS_Init2,
    MS_Ready,
    MS_Running,
    // Add other states as needed
};

class CModelState {
public:
    virtual ~CModelState () {}
    virtual void enterState(CBaseModel* model) = 0;
    virtual void handleEvent(CBaseModel* model, const e_modelStateEvent& event) = 0;
    virtual void exitState(CBaseModel* model) = 0;
    virtual e_modelState getStateID() const = 0;  // Add a method to get the state identifier
};


#endif // CMODELSTATE_H
