
#include "cbasemodel.h"
#include <QRandomGenerator>
#include <QJsonArray>
#include "cstrategyfactory.h"
#include "modelConstants.h"
#include "cmodelstateimpl.h"
#include "mandatoryFieldKeys.h"
#include "PipelineConstants.h"

CBaseModel::CBaseModel(QObject *parent): CProcessingBase_v2(parent)
    , m_Models()
    , m_ParametersMap()
    , m_InfoMap()
    , m_assetList()
    , m_genericInfo()
    , m_tmpTimer()
    , m_SelectionModel()
    , m_AlphaModel()
    , m_RebalanceModel()
    , m_RiskModel()
    , m_ExecutionModel()
    , m_ParentModel()
    , m_dbManager(parent)
    , m_ModelInfo()
    , m_availableFunds(10000.0)
    , m_usedFunds(0.0)
    , m_OpenPositionList()
{
    registerMandatoryParam(MandatoryParams::Name, "");
    registerMandatoryParam(MandatoryParams::Description, "");

    this->m_InfoMap[CIM_IsStarted] = false;
    this->m_InfoMap[CIM_IsParentActivated] = false;

//    this->getIBrokerDataProvider()->getClien().data()

    //default info
    m_ModelInfo.modelId = "";
    m_ModelInfo.modelName = "default";
    m_ModelInfo.modelDescription = "strategy";
    m_ModelInfo.createdAt = QDateTime::currentDateTime();
    m_ModelInfo.updatedAt = m_ModelInfo.createdAt;
    m_ModelInfo.status = "not active";


    //connect(signalDBManagerState)
    connect(&m_dbManager, &DBManager::signalDBManagerState, this, &CBaseModel::slotDbManagerConnectionState, Qt::AutoConnection);
    connect(m_dbManager.getDbHandler(), &DBHandler::signalModelInfoFetched, this, &CBaseModel::slotModelInfoFetched, Qt::AutoConnection);
    setState(std::make_unique<InitState>());

}

void CBaseModel::addModel(ptrGenericModelType pModel)
{
    if(nullptr != pModel)
    {
        pModel->setBrokerDataProvider(getIBrokerDataProvider());
        this->m_Models.append(pModel);
    }
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
    return m_ParametersMap[MandatoryParams::Name].toString();
}

void CBaseModel::setName(const QString& name)
{
    m_ParametersMap[MandatoryParams::Name] = name;
}

void CBaseModel::setParameters(const QVariantMap &parametersMap)
{
    QVariantMap preserved;
    for (const auto& key : m_mandatoryParamKeys) {
        preserved[key] = parametersMap.contains(key)
            ? parametersMap[key]
            : m_ParametersMap.value(key);
    }
    m_ParametersMap = parametersMap;
    for (auto it = preserved.cbegin(); it != preserved.cend(); ++it)
        m_ParametersMap[it.key()] = it.value();
}

const QVariantMap &CBaseModel::getParameters()
{
    return this->m_ParametersMap;
}

bool CBaseModel::start()
{
    connectModels();
    for (auto model : m_Models) {
        if(true == model->getActiveStatus())
        {
            model->start();
        }
    }

    if (auto _executionModel = getExecutionModel()) _executionModel->start();

    return true;
}

bool CBaseModel::stop()
{
    disconnectModels();
    for (auto model : m_Models) {
        model->stop();

        if (auto _executionModel = getExecutionModel()) _executionModel->stop();

    }
    return true;
}

QUuid CBaseModel::getId() const
{
    return m_uuid;
}

void CBaseModel::setId(const QUuid &id)
{
    this->m_uuid = id;
    m_ModelInfo.modelId = getStrUuId().c_str();
    setIsIdSet(true);
}

void CBaseModel::setActivationState(bool state)
{
    this->m_InfoMap[CIM_IsStarted] = state;
    for (auto model : m_Models) {
        if(true == getParentActivatedState())
        {
            model->setParentActivationState(state);
        }
    }

    if(true == isConnectedTotheServer())
    {
        if((true == state) && (true == getParentActivatedState()))
        {
            start();
        }
        else
        {
            stop();
        }
    }
    checkDisplayState();
}

void CBaseModel::setParentActivationState(bool state)
{
    this->m_InfoMap[CIM_IsParentActivated] = state;
    for (auto model : m_Models) {
        if((true == state) && (true == getActiveStatus()))
        {
            model->setParentActivationState(true);
        }
        else
        {
            model->setParentActivationState(false);
        }
    }
}


QJsonObject CBaseModel::toJson() const
{
    QJsonObject json;

    json["m_uuid"] = m_uuid.toString(QUuid::WithoutBraces).toStdString().c_str();

    json["modelType"] = static_cast<int>(modelType());

    // Serialize m_ParametersMap
    json["parameters"] = QJsonObject::fromVariantMap(m_ParametersMap);

    // Serialize m_InfoMap
    json["info"] = QJsonObject::fromVariantMap(m_InfoMap);

    // Serialize m_assetList
    json[Pipeline::Key::AssetList] = QJsonObject::fromVariantMap(m_assetList);

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
    auto createAndLoadModel = [this](const QJsonValue& modelJsonValue) -> std::optional<ptrGenericModelType> {
        if (modelJsonValue == QJsonValue::Undefined) return std::nullopt;

        auto modelObject = modelJsonValue.toObject();
        ModelType modelType = static_cast<ModelType>(modelObject["modelType"].toInt(static_cast<int>(ModelType::NONE)));
        ptrGenericModelType model = CStrategyFactory::createNewStrategy(modelType);

        if (model) {
            model->setBrokerDataProvider(getIBrokerDataProvider());
            model->setParentModel(this);
            model->fromJson(modelObject);
            return model;
        }
        return std::nullopt;
    };

    // from json
    setId(QUuid::fromString(json["m_uuid"].toString()));

    setParameters(json["parameters"].toObject().toVariantMap());
    m_InfoMap = json["info"].toObject().toVariantMap();
    m_assetList = json[Pipeline::Key::AssetList].toObject().toVariantMap();
    setGenericInfo(json["genericInfo"].toObject().toVariantMap());

    // Backward compat: old configs stored name in "m_Name" and/or info["name"]
    // but not in parameters["Name"]. Restore from the old locations.
    if (!json["parameters"].toObject().contains(MandatoryParams::Name)) {
        QString nameFromInfo = m_InfoMap.value("name").toString();
        if (!nameFromInfo.isEmpty()) {
            m_ParametersMap[MandatoryParams::Name] = nameFromInfo;
        } else if (json.contains("m_Name") && !json["m_Name"].toString().isEmpty()) {
            m_ParametersMap[MandatoryParams::Name] = json["m_Name"].toString();
        }
    }

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
    QVariantMap preserved;
    for (const auto& key : m_mandatoryInfoKeys) {
        preserved[key] = newGenericInfo.contains(key)
            ? newGenericInfo[key]
            : m_genericInfo.value(key);
    }
    m_genericInfo = newGenericInfo;
    for (auto it = preserved.cbegin(); it != preserved.cend(); ++it)
        m_genericInfo[it.key()] = it.value();
    checkDisplayState();
}

ModelType CBaseModel::modelType() const
{
    return ModelType::STRATEGY;
}

void CBaseModel::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient)
{
    if(nullptr != newClient)
    {
        if(nullptr != getIBrokerDataProvider())
        {
            cancelErrorNotificationSubscription();
        }

        CProcessingBase_v2::setIBrokerDataProvider(newClient);
        reqestErrorNotificationSubscription();
    }
}


void CBaseModel::onUpdateParametersSlot(const QVariantMap& parameters)
{
    setParameters(parameters);
}

void CBaseModel::onTimeoutSlot()
{
        static quint8 test = 0;
        // Seed the random generator with a unique seed value
        QRandomGenerator random(QDateTime::currentMSecsSinceEpoch() / 1000);

        // Generate a random float value between 10 and 200
        this->m_genericInfo["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
        QVariantMap tmp = this->m_assetList["SPY"].toMap();
        tmp["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
        this->m_assetList["SPY"] = tmp;

        //this->m_ParametersMap.clear();
        this->m_ParametersMap["New_One"] = "Test";
        this->m_ParametersMap["New_Two"] = random.generateDouble() * (200.0 - 10.0) + 10.0;

        this->m_assetList["KK"] = QVariantMap({{"pnl",1.0f}, {"aprice",4.0f}});
        this->m_assetList["MM"] = QVariantMap({{"pnl",3.0f}, {"aprice",5.1f}});

        this->m_genericInfo["I_New_One"] = "I_Test";
        this->m_genericInfo["I_New_Two"] = random.generateDouble() * (200.0 - 10.0) + 10.0;

        if (++test > 5)
        {
            this->m_ParametersMap.remove("New_One");
            this->m_genericInfo.remove("I_New_One");
        }

}

void CBaseModel::onUpdateServerConnectionStateSlot(bool state)
{
    qDebug() << "server state: " << ((state == true) ? "Connected" : "Disconnected");
}

void CBaseModel::slotDbManagerConnectionState(const bool state)
{
    //qDebug() << "DB state: " << ((state == true) ? "Connected" : "Disconnected");
    if(state == true) setIsDbConnected(true);
}

void CBaseModel::slotModelInfoFetched(const DbModelInfo &obj, e_queryStatus state)
{
    if(e_queryStatus::QS_VALID == state)
    {
        m_ModelInfo = obj;
        qDebug() << "Model Info: " << m_ModelInfo.modelId << m_ModelInfo.modelName;
        if (m_ParametersMap[MandatoryParams::Description].toString().isEmpty())
            m_ParametersMap[MandatoryParams::Description] = obj.modelDescription;
    }
    else if(e_queryStatus::QS_NOT_FOUND == state)
    {
        emit m_dbManager.signalAddOrUpdateDbModelInfo(m_ModelInfo);
    }

    if(getState() == e_modelState::MS_Init2) setIsDbInfoFetched(true);
}

qreal CBaseModel::getAvailableFunds() const
{
    return this->m_availableFunds;
}

void CBaseModel::setAvailableFunds(const qreal funds)
{
    this->m_availableFunds = funds;
}

QList<OpenPosition> CBaseModel::getOpenPositions() const
{
    return this->m_OpenPositionList;
}

void CBaseModel::processData(DataListPtr data)
{
    emit dataProcessed(data);
}

void CBaseModel::addSelectionModel(ptrGenericModelType pModel) {
    m_SelectionModel = pModel;
    m_SelectionModel->setBrokerDataProvider(getIBrokerDataProvider());
}

void CBaseModel::removeSelectionModel() {
    m_SelectionModel.clear();
}

void CBaseModel::addAlphaModel(ptrGenericModelType pModel) {
    m_AlphaModel = pModel;
    m_AlphaModel->setBrokerDataProvider(getIBrokerDataProvider());
}

void CBaseModel::removeAlphaModel() {
    m_AlphaModel.clear();
}

void CBaseModel::addRebalanceModel(ptrGenericModelType pModel) {
    m_RebalanceModel = pModel;
    m_RebalanceModel->setBrokerDataProvider(getIBrokerDataProvider());
}

void CBaseModel::removeRebalanceModel() {
    m_RebalanceModel.clear();
}

void CBaseModel::addRiskModel(ptrGenericModelType pModel) {
    m_RiskModel = pModel;
    m_RiskModel->setBrokerDataProvider(getIBrokerDataProvider());
}

void CBaseModel::removeRiskModel() {
    m_RiskModel.clear();
}

void CBaseModel::addExecutionModel(ptrGenericModelType pModel) {
    m_ExecutionModel = pModel;
    m_ExecutionModel->setBrokerDataProvider(getIBrokerDataProvider());
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

void CBaseModel::connectModels()
{
    QList<QSharedPointer<CBaseModel>> models = {
        m_SelectionModel.staticCast<CBaseModel>(),
        m_AlphaModel.staticCast<CBaseModel>(),
        m_RebalanceModel.staticCast<CBaseModel>(),
        m_RiskModel.staticCast<CBaseModel>(),
        m_ExecutionModel.staticCast<CBaseModel>()
    };

    QSharedPointer<CBaseModel> previousModel = nullptr;

    for (auto &currentModel : models) {
        if (!currentModel.isNull()) {
            qDebug() << "[CONNECT 1] model:" << this->getName() << "connect to " << currentModel->getName();
            QObject::connect(this, &CBaseModel::dataProcessed,
                                 currentModel.data(), &CBaseModel::processData);
            break;
        }
    }

    for (auto &currentModel : models) {
        if (!currentModel.isNull()) {
            if (!previousModel.isNull()) {
                qDebug() << "[CONNECT 2] model:" << previousModel->getName() << "connect to " << currentModel->getName();
                QObject::connect(previousModel.data(), &CBaseModel::dataProcessed,
                                 currentModel.data(), &CBaseModel::processData);
            }
            previousModel = currentModel;
        }
    }
}


void CBaseModel::disconnectModels() {
    QList<QSharedPointer<CBaseModel>> models = {
        m_SelectionModel.staticCast<CBaseModel>(),
        m_AlphaModel.staticCast<CBaseModel>(),
        m_RebalanceModel.staticCast<CBaseModel>(),
        m_RiskModel.staticCast<CBaseModel>(),
        m_ExecutionModel.staticCast<CBaseModel>()
    };

    // First, disconnect this from each model
    for (auto &currentModel : models) {
        if (!currentModel.isNull()) {
            qDebug() << "[DISCONNECT 1] model:" << this->getName() << "disconnect from " << currentModel->getName();
            QObject::disconnect(this, &CBaseModel::dataProcessed,
                                currentModel.data(), &CBaseModel::processData);
            break;
        }
    }

    // Next, disconnect each pair of models
    QSharedPointer<CBaseModel> previousModel = nullptr;
    for (auto &currentModel : models) {
        if (!currentModel.isNull()) {
            if (!previousModel.isNull()) {
                qDebug() << "[DISCONNECT 2] model:" << previousModel->getName() << "disconnect from " << currentModel->getName();
                QObject::disconnect(previousModel.data(), &CBaseModel::dataProcessed,
                                    currentModel.data(), &CBaseModel::processData);
            }
            previousModel = currentModel;
        }
    }
}

void CBaseModel::setIsDbInfoFetched(bool newIsDbInfoFetched)
{
    m_isDbInfoFetched = newIsDbInfoFetched;
    validateModelInit2();
}

void CBaseModel::setIsDbConnected(bool newIsDbConnected)
{
    m_isDbConnected = newIsDbConnected;
    validateModelInit();
}

void CBaseModel::setIsIdSet(bool newIsIdSet)
{
    m_isIdSet = newIsIdSet;
    validateModelInit();
}

inline void CBaseModel::validateModelInit()
{
    if((getState()== e_modelState::MS_Init) && (m_isIdSet == true) && (m_isDbConnected == true))
    {
        handleEvent(e_modelStateEvent::MSE_DBReady);
    }
}

void CBaseModel::validateModelInit2()
{
    if((getState()== e_modelState::MS_Init2) && (m_isDbInfoFetched == true) && (isInit2AdditionalDataReady() == true))
    {
        handleEvent(e_modelStateEvent::MSE_InitCompleted);
    }
}

bool CBaseModel::isInit2AdditionalDataReady()
{
    return true;
}

void CBaseModel::handleEvent(const e_modelStateEvent &event)
{
    if (currentState) {
        currentState->handleEvent(this, event);
    }
}

void CBaseModel::setState(std::unique_ptr<CModelState> state)
{
    if (currentState) {
        currentState->exitState(this);
    }
    currentState = std::move(state);
    if (currentState) {
        currentState->enterState(this);
    }
    checkDisplayState();
}

e_modelState CBaseModel::getState()
{
    return currentState->getStateID();
}

void CBaseModel::requestInitData()
{
    emit m_dbManager.signalGetModelInfo(getStrUuId().c_str());
}

// --- IMandatoryFields implementation ---

void CBaseModel::registerMandatoryParam(const QString& key, const QVariant& defaultValue)
{
    m_mandatoryParamKeys.insert(key);
    m_ParametersMap[key] = defaultValue;
}

void CBaseModel::registerInheritableParam(const QString& key, const QVariant& defaultValue)
{
    registerMandatoryParam(key, defaultValue);
    m_inheritableParamKeys.insert(key);
}

void CBaseModel::registerMandatoryInfo(const QString& key, const QVariant& defaultValue)
{
    m_mandatoryInfoKeys.insert(key);
    m_genericInfo[key] = defaultValue;
}

void CBaseModel::registerMandatoryAssetField(const QString& key, const QVariant& defaultValue)
{
    m_mandatoryAssetFieldKeys.insert(key);
    m_assetFieldDefaults[key] = defaultValue;
}

const QSet<QString>& CBaseModel::mandatoryParamKeys() const
{
    return m_mandatoryParamKeys;
}

const QSet<QString>& CBaseModel::mandatoryInfoKeys() const
{
    return m_mandatoryInfoKeys;
}

const QSet<QString>& CBaseModel::mandatoryAssetFieldKeys() const
{
    return m_mandatoryAssetFieldKeys;
}

CGenericModelApi* CBaseModel::findAncestor(ModelType type) const
{
    CGenericModelApi* current = m_ParentModel;
    while (current) {
        if (current->modelType() == type)
            return current;
        current = current->getParentModel();
    }
    return nullptr;
}

QVariant CBaseModel::resolvedParam(const QString& key, const QVariant& fallback) const
{
    if (!m_inheritableParamKeys.contains(key))
        return m_ParametersMap.value(key, fallback);

    if (m_ParametersMap.contains(key)) {
        const auto& val = m_ParametersMap[key];
        if (val.isValid() && !val.toString().isEmpty())
            return val;
    }
    if (m_ParentModel) {
        auto* parentBase = dynamic_cast<const CBaseModel*>(m_ParentModel);
        if (parentBase)
            return parentBase->resolvedParam(key, fallback);
    }
    return fallback;
}

double CBaseModel::aggregateChildInfo(const QString& key) const
{
    double total = 0.0;
    for (const auto& child : m_Models) {
        auto info = child->genericInfo();
        if (info.contains(key))
            total += info[key].toDouble();
    }
    return total;
}

QVariantMap CBaseModel::createAssetEntry(const QVariantMap& values) const
{
    QVariantMap entry = m_assetFieldDefaults;
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        entry[it.key()] = it.value();
    return entry;
}

bool CBaseModel::getActiveStatus() const
{
    return m_InfoMap[CIM_IsStarted].toBool();
}

bool CBaseModel::getParentActivatedState() const
{
    return m_InfoMap[CIM_IsParentActivated].toBool();
}

void CBaseModel::setParentModel(CGenericModelApi* pModel)
{
    m_ParentModel = pModel;
}

CGenericModelApi* CBaseModel::getParentModel()
{
    return m_ParentModel;
}

DisplayState CBaseModel::resolveDisplayState() const
{
    bool activated = getActiveStatus();
    e_modelState internalState = currentState ? currentState->getStateID() : e_modelState::MS_Init;

    QString statusStr;
    if (m_genericInfo.contains(MandatoryInfo::Strategy::Status))
        statusStr = m_genericInfo.value(MandatoryInfo::Strategy::Status).toString();
    else if (m_genericInfo.contains(MandatoryInfo::Account::Status))
        statusStr = m_genericInfo.value(MandatoryInfo::Account::Status).toString();
    else if (m_genericInfo.contains(MandatoryInfo::Portfolio::Status))
        statusStr = m_genericInfo.value(MandatoryInfo::Portfolio::Status).toString();

    return ModelStateUtils::resolveFromInternal(internalState, activated, statusStr);
}

void CBaseModel::checkDisplayState()
{
    DisplayState newState = resolveDisplayState();
    if (newState != m_lastDisplayState) {
        DisplayState old = m_lastDisplayState;
        m_lastDisplayState = newState;
        emit displayStateChanged(old, newState);
    }
}

