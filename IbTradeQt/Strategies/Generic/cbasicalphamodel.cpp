
#include "cbasicalphamodel.h"

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    this->setName("Base Alpha Model");
}

