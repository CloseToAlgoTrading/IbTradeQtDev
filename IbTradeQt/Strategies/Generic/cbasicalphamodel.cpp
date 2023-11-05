
#include "cbasicalphamodel.h"

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Alpha Model";
    this->setName("Base Alpha Model");
}

