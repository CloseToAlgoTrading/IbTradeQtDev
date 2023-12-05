
#include "cbasicriskmodel.h"

Q_LOGGING_CATEGORY(BasicRiskModelLog, "BasicRiskModelLog.PM");

CBasicRiskModel::CBasicRiskModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Risk Model";
    this->setName("Base Risk Model");
}

void CBasicRiskModel::processData(DataListPtr data)
{
    qCDebug(BasicRiskModelLog(), "receved and emit ->");
    emit dataProcessed(createDataList());
}

