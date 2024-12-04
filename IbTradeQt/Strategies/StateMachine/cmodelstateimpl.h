#ifndef CMODELSTATEIMPL_H
#define CMODELSTATEIMPL_H

#include "cbasemodel.h"
#include "cmodelstate.h"
#include "qdebug.h"

enum class e_modelStateEvent{
    MSE_InitCompleted = 0,
    MSE_DBReady,
    MSE_StartProcessing,
    MSE_PauseProcessing
};

class InitState : public CModelState {
public:
    void enterState(CBaseModel* model) override;
    void handleEvent(CBaseModel* model, const e_modelStateEvent& event) override;
    void exitState(CBaseModel* model) override;
    e_modelState getStateID() const override;
};

class Init2State : public CModelState {
public:
    void enterState(CBaseModel* model) override;
    void handleEvent(CBaseModel* model, const e_modelStateEvent& event) override;
    void exitState(CBaseModel* model) override;
    e_modelState getStateID() const override;
};


class ReadyState : public CModelState {
public:
    void enterState(CBaseModel* model) override;
    void handleEvent(CBaseModel* model, const e_modelStateEvent& event) override;
    void exitState(CBaseModel* model) override;
    e_modelState getStateID() const override;
};

class RunningState : public CModelState {
public:
    void enterState(CBaseModel* model) override;
    void handleEvent(CBaseModel* model, const e_modelStateEvent& event) override;
    void exitState(CBaseModel* model) override;
    e_modelState getStateID() const override;
};

/************* Init State ***********************/
void InitState::enterState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Entering Collecting State";
    // Initialize data collection
}

void InitState::handleEvent(CBaseModel* model, const e_modelStateEvent& event) {
    if (event == e_modelStateEvent::MSE_DBReady) {
        //qDebug() << "Data collected, transitioning to Ready State";
        model->setState(std::make_unique<Init2State>());
    }
}

void InitState::exitState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Exiting Collecting State";
    // Clean up resources if needed
}

e_modelState InitState::getStateID() const {
    return e_modelState::MS_Init;
}
/**************************************************/

/************* Init State ***********************/
void Init2State::enterState(CBaseModel* model) {
    //qDebug() << "Entering Collecting State";
    // Initialize data collection
    model->requestInitData();
}

void Init2State::handleEvent(CBaseModel* model, const e_modelStateEvent& event) {
    if (event == e_modelStateEvent::MSE_InitCompleted) {
        //qDebug() << "Data collected, transitioning to Ready State";
        model->setState(std::make_unique<ReadyState>());
    }
}

void Init2State::exitState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Exiting Collecting State";
    // Clean up resources if needed
}

e_modelState Init2State::getStateID() const {
    return e_modelState::MS_Init2;
}
/**************************************************/



/************* Ready State ***********************/
void ReadyState::enterState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Entering Ready State";
    // Initialize ready state
}

void ReadyState::handleEvent(CBaseModel* model, const e_modelStateEvent& event){
    if (event == e_modelStateEvent::MSE_StartProcessing) {
        //qDebug() << "Starting strategy, transitioning to Running State";
        model->setState(std::make_unique<RunningState>());
    }
}

void ReadyState::exitState(CBaseModel* model){
    Q_UNUSED(model)
    qDebug() << "Exiting Ready State";
    // Clean up resources if needed
}

e_modelState ReadyState::getStateID() const {
    return e_modelState::MS_Ready;
}
/**************************************************/

/************* Ready State ***********************/
void RunningState::enterState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Entering Running State";
    // Initialize running state
}

void RunningState::handleEvent(CBaseModel* model, const e_modelStateEvent& event) {
    Q_UNUSED(model)
    if (event == e_modelStateEvent::MSE_PauseProcessing) {
        //qDebug() << "Pausing strategy, transitioning to Ready State";
        model->setState(std::make_unique<ReadyState>());
    }
}

void RunningState::exitState(CBaseModel* model) {
    Q_UNUSED(model)
    //qDebug() << "Exiting Running State";
    // Clean up resources if needed
}

e_modelState RunningState::getStateID() const {
    return e_modelState::MS_Running;
}
/**************************************************/

#endif // CMODELSTATEIMPL_H
