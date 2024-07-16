
#include "cmomentum.h"
#include "UnifiedModelData.h"
#include "modelConstants.h"
#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_strategyInfo()
{
    m_Name = "Momentum";
    this->setName("Momentum");

    this->m_ParametersMap[CPM_BP] = 10000.0f;

    //connect(&m_dbManager, &DBManager::signalOpenPositionsFetched, this, &CBaseRebalanceModel::slotOpenPositionsFetched, Qt::AutoConnection);

    //emit m_dbManager.signalAddOrUpdateDbStrategyInfo(m_strategyInfo);
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
    /*m_strategyInfo.strategyId = m_uuid.toString(QUuid::WithoutBraces).toStdString().c_str();
    m_strategyInfo.strategyName = "Momentum";
    m_strategyInfo.strategyDescription = "A simple momentum strategy";
    m_strategyInfo.initialBP = 10000.0f;
    m_strategyInfo.createdAt = QDateTime::currentDateTime();
    m_strategyInfo.updatedAt = m_strategyInfo.createdAt;
    m_strategyInfo.currency = "USD";
    m_strategyInfo.status = "active";
*/
    qDebug() << "DB state: " << ((state == true) ? "Connected" : "Disconnected");
    //emit m_dbManager.signalAddOrUpdateDbStrategyInfo(m_strategyInfo);
}


