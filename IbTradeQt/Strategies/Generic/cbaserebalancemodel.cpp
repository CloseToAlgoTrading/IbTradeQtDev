
#include "cbaserebalancemodel.h"

Q_LOGGING_CATEGORY(BasicRebalanceModelLog, "BasicRebalanceModelLog.PM");

CBaseRebalanceModel::CBaseRebalanceModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Rebalance Model";
    this->setName("Base Rebalance Model");

}

