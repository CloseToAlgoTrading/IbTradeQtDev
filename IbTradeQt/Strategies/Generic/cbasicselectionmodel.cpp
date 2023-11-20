
#include "cbasicselectionmodel.h"
#include "UnifiedModelData.h"

Q_LOGGING_CATEGORY(BasicSelectionModelLog, "BasicSelectionModel.PM");

CBasicSelectionModel::CBasicSelectionModel(QObject *parent) : CBasicStrategy_V2(parent)
{
    m_Name = "Base Selection Model";
    this->setName("Base Selection Model");
    this->m_genericInfo.clear();
    this->m_ParametersMap.clear();
    this->m_ParametersMap["Selected_Assets"] = QString("NVDA");
    this->m_genericInfo["Selected_Assets"] = "NVDA";
    m_pAssetList = createDataList();
    m_pAssetList->append(UnifiedModelData("NVDA", 0, 0, 0));
}

bool CBasicSelectionModel::start()
{
    qDebug() << "Selection Start - emit signal: " ;
    emit dataProcessed(m_pAssetList);
    return true;
}

void CBasicSelectionModel::setParameters(const QVariantMap &parametersMap)
{
    CBaseModel::setParameters(parametersMap);
    auto assets = this->m_ParametersMap["Selected_Assets"].toString();
    assets.remove(" ");
    auto assetList = assets.split(",");
    m_pAssetList->clear();
    for (const QString &str : assetList)
    {
        m_pAssetList->append(UnifiedModelData(str, 0, 0, 0));
    }
}


