
#include "cbasicselectionmodel.h"
#include "Backtest/AssetUniverseInput.h"
#include "UnifiedModelData.h"
#include "mandatoryFieldKeys.h"

Q_LOGGING_CATEGORY(BasicSelectionModelLog, "BasicSelectionModel.PM");

CBasicSelectionModel::CBasicSelectionModel(QObject *parent) : CBaseModel(parent)
{
    registerMandatoryParam(MandatoryParams::Name, "Base Selection Model");
    this->m_ParametersMap["Selected_Assets"] = "";
    this->m_genericInfo["Selected_Assets"] = "";
    m_pAssetList = createDataList();
}

void CBasicSelectionModel::setParameters(const QVariantMap &parametersMap)
{
    CBaseModel::setParameters(parametersMap);
    auto assets = this->m_ParametersMap["Selected_Assets"].toString();
    this->m_genericInfo["Selected_Assets"] = this->m_ParametersMap["Selected_Assets"];
    const auto parsed = AssetUniverseInput::parseLine(assets);

    m_pAssetList->clear();
    for (const QString& str : parsed.symbolOrder)
        m_pAssetList->append(UnifiedModelData(str, DIRECTION_UNDEFINED, 0, 0));
}

void CBasicSelectionModel::processData(DataListPtr data)
{
    Q_UNUSED(data);
    qCDebug(BasicSelectionModelLog) << "Selection processData - emit signal: ";

    auto assets = this->m_ParametersMap["Selected_Assets"].toString();
    const auto parsed = AssetUniverseInput::parseLine(assets);
    m_pAssetList->clear();
    for (const QString& str : parsed.symbolOrder)
        m_pAssetList->append(UnifiedModelData(str, DIRECTION_UNDEFINED, 0, 0));

    emit dataProcessed(m_pAssetList);
}


