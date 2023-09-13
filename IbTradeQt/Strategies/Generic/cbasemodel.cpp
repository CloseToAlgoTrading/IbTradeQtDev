
#include "cbasemodel.h"
#include <QRandomGenerator>
#include <QJsonArray>
#include "cstrategyfactory.h"

CBaseModel::CBaseModel(QObject *parent): CProcessingBase_v2(parent)
    , m_Models()
    , m_ParametersMap()
    , m_InfoMap()
    //    , m_DataProvider()
    , m_assetList()
    , m_genericInfo()
    , m_tmpTimer()
    , m_SelectionModel()
    , m_AlphaModel()
    , m_RebalanceModel()
    , m_RiskModel()
    , m_ExecutionModel()
{
    this->m_genericInfo["test_pnl"] = 0.2f;
    //this->m_assetList["test_SPY"] = QVariantMap({{"pnl",23.0f}, {"aprice",100.0f}});

    QObject::connect(&m_tmpTimer, &QTimer::timeout, this, &CBaseModel::onTimeoutSlot);
}

void CBaseModel::addModel(ptrGenericModelType pModel)
{
    this->m_Models.append(pModel);
}

void CBaseModel::removeModel(ptrGenericModelType pModel)
{
    if (pModel) {
        this->m_Models.removeOne(pModel);
    }
}

QList<ptrGenericModelType>& CBaseModel::getModels()
{
    return this->m_Models;
}

QString CBaseModel::getName()
{
    return this->m_InfoMap["name"].toString();
}

void CBaseModel::setName(const QString& name)
{
    this->m_InfoMap["name"] = name;
}

void CBaseModel::setParameters(const QVariantMap &parametersMap)
{
    this->m_ParametersMap = parametersMap;
    //emit onUpdateParametersSignal(this->m_ParametersMap);
}

const QVariantMap &CBaseModel::getParameters()
{
    return this->m_ParametersMap;
}

bool CBaseModel::start()
{
    this->m_InfoMap["IsStarted"] = true;
    return true;
}

bool CBaseModel::stop()
{
    this->m_InfoMap["IsStarted"] = false;
    return false;
}


QJsonObject CBaseModel::toJson() const
{
    QJsonObject json;

    // Serialize m_Name
    json["modelType"] = static_cast<int>(modelType());

    // Serialize m_ParametersMap
    json["parameters"] = QJsonObject::fromVariantMap(m_ParametersMap);

    // Serialize m_InfoMap
    json["info"] = QJsonObject::fromVariantMap(m_InfoMap);

    // Serialize m_assetList
    json["assetList"] = QJsonObject::fromVariantMap(m_assetList);

    // Serialize m_genericInfo
    json["genericInfo"] = QJsonObject::fromVariantMap(m_genericInfo);

    // Serialize m_Models
    QJsonArray modelsArray;
    for (const auto& model : m_Models) {
        modelsArray.append(model->toJson());
    }
    json["models"] = modelsArray;

    if(nullptr != this->m_SelectionModel) {
        json["selectionModel"] = m_SelectionModel->toJson();
    }
    if(nullptr != this->m_AlphaModel) {
        json["alphaModel"] = m_AlphaModel->toJson();
    }
    if(nullptr != this->m_RebalanceModel) {
        json["rebalanceModel"] = m_RebalanceModel->toJson();
    }
    if(nullptr != this->m_RiskModel) {
        json["riskModel"] = m_RiskModel->toJson();
    }
    if(nullptr != this->m_ExecutionModel) {
        json["executionModel"] = m_ExecutionModel->toJson();
    }

    return json;
}


void CBaseModel::fromJson(const QJsonObject &json)
{
    // helper function
    auto createAndLoadModel = [](const QJsonValue& modelJsonValue) -> std::optional<ptrGenericModelType> {
        if (modelJsonValue == QJsonValue::Undefined) return std::nullopt;

        auto modelObject = modelJsonValue.toObject();
        ModelType modelType = static_cast<ModelType>(modelObject["modelType"].toInt(static_cast<int>(ModelType::NONE)));
        ptrGenericModelType model = CStrategyFactory::createNewStrategy(modelType);

        if (model) {
            model->fromJson(modelObject);
            return model;
        }
        return std::nullopt;
    };

    // from json
    m_ParametersMap = json["parameters"].toObject().toVariantMap();
    m_InfoMap = json["info"].toObject().toVariantMap();
    m_assetList = json["assetList"].toObject().toVariantMap();
    m_genericInfo = json["genericInfo"].toObject().toVariantMap();

    m_Models.clear();
    QJsonArray modelsArray = json["models"].toArray();

    for (const auto& modelJson : modelsArray) {
        if(auto model = createAndLoadModel(modelJson)) {
            m_Models.append(*model);
        }
    }

    if (auto model = createAndLoadModel(json["selectionModel"])) m_SelectionModel = *model;
    if (auto model = createAndLoadModel(json["alphaModel"])) m_AlphaModel = *model;
    if (auto model = createAndLoadModel(json["rebalanceModel"])) m_RebalanceModel = *model;
    if (auto model = createAndLoadModel(json["riskModel"])) m_RiskModel = *model;
    if (auto model = createAndLoadModel(json["executionModel"])) m_ExecutionModel = *model;
}


QVariantMap CBaseModel::assetList() const
{
    return m_assetList;
}

void CBaseModel::setAssetList(const QVariantMap &newAssetList)
{
    m_assetList = newAssetList;
}

QVariantMap CBaseModel::genericInfo() const
{
    return m_genericInfo;
}

void CBaseModel::setGenericInfo(const QVariantMap &newGenericInfo)
{
    m_genericInfo = newGenericInfo;
}

ModelType CBaseModel::modelType() const
{
    return ModelType::STRATEGY;
}

void CBaseModel::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient)
{
    CProcessingBase_v2::setIBrokerDataProvider(newClient);
}


void CBaseModel::onUpdateParametersSlot(const QVariantMap& parameters)
{
    this->m_ParametersMap = parameters;
}

void CBaseModel::onTimeoutSlot()
{
    //    static quint8 test = 0;
    //    // Seed the random generator with a unique seed value
    //    QRandomGenerator random(QDateTime::currentMSecsSinceEpoch() / 1000);

    //    // Generate a random float value between 10 and 200
    //    this->m_genericInfo["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
    //    QVariantMap tmp = this->m_assetList["SPY"].toMap();
    //    tmp["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
    //    this->m_assetList["SPY"] = tmp;

    //    //this->m_ParametersMap.clear();
    //    this->m_ParametersMap["New_One"] = "Test";
    //    this->m_ParametersMap["New_Two"] = random.generateDouble() * (200.0 - 10.0) + 10.0;

    //    this->m_assetList["KK"] = QVariantMap({{"pnl",1.0f}, {"aprice",4.0f}});
    //    this->m_assetList["MM"] = QVariantMap({{"pnl",3.0f}, {"aprice",5.1f}});

    //    this->m_genericInfo["I_New_One"] = "I_Test";
    //    this->m_genericInfo["I_New_Two"] = random.generateDouble() * (200.0 - 10.0) + 10.0;

    //    if (++test > 5)
    //    {
    //        this->m_ParametersMap.remove("New_One");
    //        this->m_genericInfo.remove("I_New_One");
    //    }

}

void CBaseModel::addSelectionModel(ptrGenericModelType pModel) {
    m_SelectionModel = pModel;
}

void CBaseModel::removeSelectionModel() {
    m_SelectionModel.clear();
}

void CBaseModel::addAlphaModel(ptrGenericModelType pModel) {
    m_AlphaModel = pModel;
}

void CBaseModel::removeAlphaModel() {
    m_AlphaModel.clear();
}

void CBaseModel::addRebalanceModel(ptrGenericModelType pModel) {
    m_RebalanceModel = pModel;
}

void CBaseModel::removeRebalanceModel() {
    m_RebalanceModel.clear();
}

void CBaseModel::addRiskModel(ptrGenericModelType pModel) {
    m_RiskModel = pModel;
}

void CBaseModel::removeRiskModel() {
    m_RiskModel.clear();
}

void CBaseModel::addExecutionModel(ptrGenericModelType pModel) {
    m_ExecutionModel = pModel;
}

void CBaseModel::removeExecutionModel() {
    m_ExecutionModel.clear();
}

ptrGenericModelType CBaseModel::getSelectionModel() {
    return m_SelectionModel;
}

ptrGenericModelType CBaseModel::getAlphaModel() {
    return m_AlphaModel;
}

ptrGenericModelType CBaseModel::getRebalanceModel() {
    return m_RebalanceModel;
}

ptrGenericModelType CBaseModel::getRiskModel() {
    return m_RiskModel;
}

ptrGenericModelType CBaseModel::getExecutionModel() {
    return m_ExecutionModel;
}
