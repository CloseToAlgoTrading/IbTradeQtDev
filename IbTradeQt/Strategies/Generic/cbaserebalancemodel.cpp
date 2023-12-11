
#include "cbaserebalancemodel.h"

Q_LOGGING_CATEGORY(BasicRebalanceModelLog, "BasicRebalanceModelLog.PM");

CBaseRebalanceModel::CBaseRebalanceModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Rebalance Model";
    this->setName("Base Rebalance Model");

}

void CBaseRebalanceModel::processData(DataListPtr data)
{
    qCDebug(BasicRebalanceModelLog(), "receved and emit ->");
    qreal _bp = 0.0f;
    if(nullptr != m_ParentModel)
    {
        auto temp = m_ParentModel->getParameters();
        _bp = temp["BP"].toReal();
    }

    for (const auto & item : *data) {
//        qCDebug(BasicRebalanceModelLog(), "%s : %d", item.symbol.toStdString().c_str(), item.direction);
        qCDebug(BasicRebalanceModelLog(), "%s : %d --> %.2f", item.symbol.toStdString().c_str(), item.direction, _bp);
    }

    emit dataProcessed(data);
}

