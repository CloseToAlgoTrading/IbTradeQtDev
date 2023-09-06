
#include "cbasicriskmodel.h"

Q_LOGGING_CATEGORY(BasicRiskModelLog, "BasicRiskModelLog.PM");

CBasicRiskModel::CBasicRiskModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    this->setName("Base Risk Model");
}

