
#include "cbaserebalancemodel.h"
#include "modelConstants.h"
#include "cbasicstrategy_V2.h"
#include "mandatoryFieldKeys.h"

Q_LOGGING_CATEGORY(BasicRebalanceModelLog, "BasicRebalanceModelLog.PM");

CBaseRebalanceModel::CBaseRebalanceModel(QObject *parent)
    : CBaseModel{parent}
    //, m_dbManager(parent)
{
    registerMandatoryParam(MandatoryParams::Name, "Base Rebalance Model");

    connect(&m_dbManager, &DBManager::signalOpenPositionsFetched, this, &CBaseRebalanceModel::slotOpenPositionsFetched, Qt::AutoConnection);

}

void CBaseRebalanceModel::processData(DataListPtr data)
{

    qCDebug(BasicRebalanceModelLog(), "receved and emit ->");
//    m_dbManager.getOpenPositions(m_ParentModel->getId().toString(QUuid::WithoutBraces));

    // get open positions
    if(nullptr != m_ParentModel)
    {
        CBasicStrategy_V2* parentModel = static_cast<CBasicStrategy_V2*>(this->getParentModel());

//        parentModel->m_positionMap;
        if(0 < data->length())
        {
            qreal _bp = 0.0f;
            auto temp = m_ParentModel->getParameters();
            _bp = temp[CPM_BP].toReal();

            auto n = _bp / data->length();

            for (auto & item : *data) {
                auto amount = static_cast<quint32>(n / item.currentPrice);
                qCDebug(BasicRebalanceModelLog(), "%s : %d : %f --> %.2f -> %f -> %d", item.symbol.toStdString().c_str(), item.direction, item.currentPrice, _bp, n, amount);
                item.amount = amount;
            }
            emit dataProcessed(data);
        }
    }
}

void CBaseRebalanceModel::slotOpenPositionsFetched(const QList<OpenPosition> &positions)
{
    qDebug() << "-------start---------";
    for (auto pos : positions) {
        qDebug() << pos.symbol << pos.quantity << pos.price << pos.date;
    }
    qDebug() << "-------stop---------";
}

