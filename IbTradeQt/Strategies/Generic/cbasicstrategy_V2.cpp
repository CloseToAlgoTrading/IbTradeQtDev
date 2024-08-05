#include "cbasicstrategy_V2.h"

CBasicStrategy_V2::CBasicStrategy_V2(QObject *parent): CBaseModel(parent)
    , m_StrategyData()
{
    m_Name = "CBasicStrategy_V2";

    connect(m_dbManager.getDbHandler(), &DBHandler::signalOpenPositionsFetched, this, &CBasicStrategy_V2::slotOpenPositionsFetched, Qt::AutoConnection);
    connect(m_dbManager.getDbHandler(), &DBHandler::signalStrategyDataFetched, this, &CBasicStrategy_V2::slotStrategyDataFetched, Qt::AutoConnection);
}

void CBasicStrategy_V2::requestInitData()
{
    /* Base Methods */
    CBaseModel::requestInitData();

    /* request strategy info */
    emit m_dbManager.signalGetStrategyData(getStrUuId().c_str());
    /* request Open Position */
    emit m_dbManager.signalGetOpenPositionsQuery(getStrUuId().c_str());
}

void CBasicStrategy_V2::slotStrategyDataFetched(const DbStrategyData &obj, e_queryStatus state)
{
    switch (state) {
    case QS_VALID:
        if(m_StrategyData.strategyId == obj.strategyId)
        {
            m_StrategyData = obj;
            m_genericInfo["PnL (%)"] = m_StrategyData.pnlPercentage;
            m_genericInfo["Reilized PnL"] = m_StrategyData.realizedPnL;
            m_genericInfo["Unreilized PnL"] = m_StrategyData.unrealizedPnL;
            m_genericInfo["Available BP"] = m_StrategyData.availableBP;
            m_genericInfo["Used BP"] = m_StrategyData.usedBP;
            m_genericInfo["Fees"] = m_StrategyData.fees;
        }
        break;
    case QS_NOT_FOUND:
        emit m_dbManager.signalAddOrUpdateDbStrategyData(m_StrategyData);
        break;
    default:
        break;
    }
    qDebug() << "Data Fetched: " << ((state == QS_VALID) ? "Valid" : "Not Valid");
}

void CBasicStrategy_V2::slotOpenPositionsFetched(const QList<OpenPosition> &positions, e_queryStatus state)
{
    switch (state) {
    case QS_VALID:
        this->m_assetList.clear();
        for (QList<OpenPosition>::const_iterator it = positions.begin(); it != positions.cend(); ++it) {
            const OpenPosition &pos = *it;
            // Process each position
            this->m_assetList[pos.symbol] = QVariantMap({{"pnl",pos.pnl}, {"aprice",pos.price}, {"quantity",pos.quantity}});
        }
        break;
    case QS_NOT_FOUND:
        break;
    default:
        break;
    }
}
