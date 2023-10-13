
#include "cbasicselectionmodel.h"

Q_LOGGING_CATEGORY(BasicSelectionModelLog, "BasicSelectionModel.PM");

CBasicSelectionModel::CBasicSelectionModel(QObject *parent) : CBasicStrategy_V2(parent)
{
    this->setName("Base Selection Model");
    this->m_genericInfo.clear();
    this->m_genericInfo["Selected_Assets"] = QString("AAPL, MSFT, NVDA, SPY");
    this->m_ParametersMap["test"] = 12;
}

QStringList CBasicSelectionModel::getAssets()
{
    return QStringList{"AAPL", "MSFT", "NVDA", "SPY"};
}

