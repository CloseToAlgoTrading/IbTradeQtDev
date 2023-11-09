
#include "cbasicselectionmodel.h"

Q_LOGGING_CATEGORY(BasicSelectionModelLog, "BasicSelectionModel.PM");

CBasicSelectionModel::CBasicSelectionModel(QObject *parent) : CBasicStrategy_V2(parent)
    , m_AssetList()
{
    m_Name = "Base Selection Model";
    this->setName("Base Selection Model");
    this->m_genericInfo.clear();
    //this->m_genericInfo["Selected_Assets"] = QString("AAPL, MSFT, NVDA, SPY");
    this->m_ParametersMap.clear();
    this->m_ParametersMap["Selected_Assets"] = QString("AAPL, MSFT, NVDA, SPY");
}

QStringList CBasicSelectionModel::getAssets()
{
    return m_AssetList;
}

void CBasicSelectionModel::setParameters(const QVariantMap &parametersMap)
{
    CBaseModel::setParameters(parametersMap);
    auto assets = this->m_ParametersMap["Selected_Assets"].toString();
    assets.remove(" ");
    m_AssetList = assets.split(",");
}


