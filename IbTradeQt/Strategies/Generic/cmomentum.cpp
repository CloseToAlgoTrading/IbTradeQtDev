
#include "cmomentum.h"
#include "UnifiedModelData.h"
#include "modelConstants.h"
#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

#define CPM_EXECUTION_CYCLE_TIME "Cycle Time"

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_triggerTimer()
{
    m_Name = "Momentum";
    this->setName("Momentum");
    this->m_ParametersMap[CPM_BP] = 10000.0f;
//    this->m_ParametersMap[CPM_EXECUTION_CYCLE_TIME] = 10000;

    QObject::connect(&m_triggerTimer, &QTimer::timeout, this, &cMomentum::onTimeoutSlot);
}

bool cMomentum::start()
{
    if(!isConnectedTotheServer()) return false;
    this->m_ParametersMap[CPM_EXECUTION_CYCLE_TIME] = 10000;
    CBasicStrategy_V2::start();
    auto m_pAssetList = createDataList();
    emit dataProcessed(m_pAssetList);
    auto timeValue = this->m_ParametersMap[CPM_EXECUTION_CYCLE_TIME].toInt();
    m_triggerTimer.start(timeValue);
    return true;
}

bool cMomentum::stop()
{
    m_triggerTimer.stop();
    return CBasicStrategy_V2::stop();
}

void cMomentum::setId(const QUuid &id)
{
    CBaseModel::setId(id);
    m_StrategyData.strategyId = getStrUuId().c_str();
}


void cMomentum::slotDbManagerConnectionState(const bool state)
{
    CBaseModel::slotDbManagerConnectionState(state);
    qDebug() << "DB state: " << ((state == true) ? "Connected" : "Disconnected");

}

void cMomentum::onTimeoutSlot()
{
    // Start data processing
    //emit dataProcessed(nullptr);
    qDebug() << "Timer Triggered";
}

