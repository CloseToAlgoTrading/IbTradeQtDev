
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
//    auto temp = m_ParentModel->getParameters();
    for (const auto & item : *data) {
        qCDebug(BasicRebalanceModelLog(), "%s : %d", item.symbol.toStdString().c_str(), item.direction);
        //qCDebug(BasicRebalanceModelLog(), "%s : %d --> %f", item.symbol.toStdString().c_str(), item.direction, temp["BP"].toFloat());
    }

    emit dataProcessed(data);
}

