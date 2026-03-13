
#include "cbasicriskmodel.h"
#include "mandatoryFieldKeys.h"

Q_LOGGING_CATEGORY(BasicRiskModelLog, "BasicRiskModelLog.PM");

CBasicRiskModel::CBasicRiskModel(QObject *parent)
    : CBaseModel{parent}
{
    registerMandatoryParam(MandatoryParams::Name, "Base Risk Model");
}

void CBasicRiskModel::processData(DataListPtr data)
{
    qCDebug(BasicRiskModelLog(), "receved and emit ->");
    emit dataProcessed(createDataList());
}

