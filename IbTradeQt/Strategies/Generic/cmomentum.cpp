
#include "cmomentum.h"
#include "UnifiedModelData.h"
#include "modelConstants.h"
#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
//    , m_ModelInfo()
{
    m_Name = "Momentum";
    this->setName("Momentum");

    this->m_ParametersMap[CPM_BP] = 10000.0f;

    //connect(&m_dbManager, &DBManager::signalOpenPositionsFetched, this, &CBaseRebalanceModel::slotOpenPositionsFetched, Qt::AutoConnection);

    //emit m_dbManager.signalAddOrUpdateDbModelInfo(m_ModelInfo);
}

bool cMomentum::start()
{
    if(!isConnectedTotheServer()) return false;
    CBasicStrategy_V2::start();
    auto m_pAssetList = createDataList();
    emit dataProcessed(m_pAssetList);
    return true;
}

bool cMomentum::stop()
{
    return CBasicStrategy_V2::stop();
}

//void cMomentum::requestInitData()
//{
    /* request Open Position */

    /* request strategy info */
    /* initial BP */
    /* available BP */
    /* used BP */
    /* realized P&L */
    /* unrealized P&L */
    /* P&L % */
    /* fees */
//}

void cMomentum::slotDbManagerConnectionState(const bool state)
{
    CBaseModel::slotDbManagerConnectionState(state);
    qDebug() << "DB state: " << ((state == true) ? "Connected" : "Disconnected");

    // m_ModelInfo.modelId = m_uuid.toString(QUuid::WithoutBraces).toStdString().c_str();
    // m_ModelInfo.modelName = "Momentum";
    // m_ModelInfo.modelDescription = "A simple momentum strategy";
    // m_ModelInfo.createdAt = QDateTime::currentDateTime();
    // m_ModelInfo.updatedAt = m_ModelInfo.createdAt;
    // m_ModelInfo.status = "active";
    // emit m_dbManager.signalAddOrUpdateDbModelInfo(m_ModelInfo);


}


