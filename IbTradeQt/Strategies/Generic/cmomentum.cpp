
#include "cmomentum.h"
#include "UnifiedModelData.h"
#include "modelConstants.h"
#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
    // , m_StrategyData()
{
    m_Name = "Momentum";
    this->setName("Momentum");

    this->m_ParametersMap[CPM_BP] = 10000.0f;

    // connect(m_dbManager.getDbHandler(), &DBHandler::signalOpenPositionsFetched, this, &cMomentum::slotOpenPositionsFetched, Qt::AutoConnection);
    // connect(m_dbManager.getDbHandler(), &DBHandler::signalStrategyDataFetched, this, &cMomentum::slotStrategyDataFetched, Qt::AutoConnection);

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

void cMomentum::setId(const QUuid &id)
{
    CBaseModel::setId(id);
    m_StrategyData.strategyId = getStrUuId().c_str();
}

// void cMomentum::requestInitData()
// {
//     /* Base Methods */
//     CBaseModel::requestInitData();
//     /* request Open Position */

//     /* request strategy info */
//     emit m_dbManager.signalGetStrategyData(getStrUuId().c_str());
//     emit m_dbManager.signalGetOpenPositionsQuery(getStrUuId().c_str());
// }

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

// void cMomentum::slotStrategyDataFetched(const DbStrategyData &obj, e_queryStatus state)
// {
//     switch (state) {
//     case QS_VALID:
//         if(m_StrategyData.strategyId == obj.strategyId)
//         {
//             m_StrategyData = obj;
//             m_genericInfo["PnL (%)"] = m_StrategyData.pnlPercentage;
//             m_genericInfo["Reilized PnL"] = m_StrategyData.realizedPnL;
//             m_genericInfo["Unreilized PnL"] = m_StrategyData.unrealizedPnL;
//             m_genericInfo["Available BP"] = m_StrategyData.availableBP;
//             m_genericInfo["Used BP"] = m_StrategyData.usedBP;
//             m_genericInfo["Fees"] = m_StrategyData.fees;
//         }
//         break;
//     case QS_NOT_FOUND:
//         emit m_dbManager.signalAddOrUpdateDbStrategyData(m_StrategyData);
//         break;
//     default:
//         break;
//     }
//     qDebug() << "Data Fetched: " << ((state == QS_VALID) ? "Valid" : "Not Valid");
// }

// void cMomentum::slotOpenPositionsFetched(const QList<OpenPosition> &positions, e_queryStatus state)
// {
//     switch (state) {
//     case QS_VALID:
//         this->m_assetList.clear();
//         for (QList<OpenPosition>::const_iterator it = positions.begin(); it != positions.cend(); ++it) {
//             const OpenPosition &pos = *it;
//             // Process each position
//             this->m_assetList[pos.symbol] = QVariantMap({{"pnl",pos.pnl}, {"aprice",pos.price}, {"quantity",pos.quantity}});
//         }
//         break;
//     case QS_NOT_FOUND:
//         break;
//     default:
//         break;
//     }
// }


