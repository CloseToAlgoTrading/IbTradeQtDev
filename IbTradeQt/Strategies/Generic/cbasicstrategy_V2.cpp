#include "cbasicstrategy_V2.h"
#include <QRandomGenerator>
#include <QJsonArray>
#include "cstrategyfactory.h"

CBasicStrategy_V2::CBasicStrategy_V2(QObject *parent): CProcessingBase_v2(parent)
    , m_Models()
    , m_ParametersMap()
    , m_InfoMap()
    , m_DataProvider()
    , m_assetList()
    , m_genericInfo()
    , m_tmpTimer()
{
    this->m_genericInfo["pnl"] = 95.2f;
    this->m_genericInfo["pnlpnc"] = "2.1 %";


    this->m_assetList["SPY"] = QVariantMap({{"pnl",23.0f}, {"aprice",100.0f}});
    this->m_assetList["V"] = QVariantMap({{"pnl",2.0f}, {"aprice",320.1f}});

    QObject::connect(&m_tmpTimer, &QTimer::timeout, this, &CBasicStrategy_V2::onTimeoutSlot);
    m_tmpTimer.start(100);

}

void CBasicStrategy_V2::addModel(ptrGenericModelType pModel)
{
    this->m_Models.append(pModel);
}

void CBasicStrategy_V2::removeModel(ptrGenericModelType pModel)
{
    if (pModel) {
        this->m_Models.removeOne(pModel);
    }
}

QList<ptrGenericModelType>& CBasicStrategy_V2::getModels()
{
    return this->m_Models;
}

QString CBasicStrategy_V2::getName()
{
    return this->m_InfoMap["name"].toString();
}

void CBasicStrategy_V2::setName(const QString& name)
{
    this->m_InfoMap["name"] = name;
}

void CBasicStrategy_V2::setParameters(const QVariantMap &parametersMap)
{
    this->m_ParametersMap = parametersMap;
    //emit onUpdateParametersSignal(this->m_ParametersMap);
}

const QVariantMap &CBasicStrategy_V2::getParameters()
{
    return this->m_ParametersMap;
}

bool CBasicStrategy_V2::start()
{
    this->m_InfoMap["IsStarted"] = true;
    return true;
}

bool CBasicStrategy_V2::stop()
{
    this->m_InfoMap["IsStarted"] = false;
    return false;
}

void CBasicStrategy_V2::setBrokerInterface(QSharedPointer<IBrokerAPI> interface)
{
    this->m_DataProvider.setClien(interface);
}

QJsonObject CBasicStrategy_V2::toJson() const
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

    return json;
}

void CBasicStrategy_V2::fromJson(const QJsonObject &json)
{

    // Deserialize m_ParametersMap
    m_ParametersMap = json["parameters"].toObject().toVariantMap();

    // Deserialize m_InfoMap
    m_InfoMap = json["info"].toObject().toVariantMap();

    // Deserialize m_assetList
    m_assetList = json["assetList"].toObject().toVariantMap();

    // Deserialize m_genericInfo
    m_genericInfo = json["genericInfo"].toObject().toVariantMap();

    // Deserialize m_Models
    m_Models.clear();
    QJsonArray modelsArray = json["models"].toArray();
    for (const auto& modelJson : modelsArray) {
        ModelType modelType = static_cast<ModelType>(modelJson.toObject()["modelType"].toInt());
        ptrGenericModelType model = CStrategyFactory::createNewStrategy(modelType);
        model->fromJson(modelJson.toObject());
        m_Models.append(model);
    }

}

QVariantMap CBasicStrategy_V2::assetList() const
{
    return m_assetList;
}

void CBasicStrategy_V2::setAssetList(const QVariantMap &newAssetList)
{
    m_assetList = newAssetList;
}

QVariantMap CBasicStrategy_V2::genericInfo() const
{
    return m_genericInfo;
}

void CBasicStrategy_V2::setGenericInfo(const QVariantMap &newGenericInfo)
{
    m_genericInfo = newGenericInfo;
}

ModelType CBasicStrategy_V2::modelType() const
{
    return ModelType::STRATEGY;
}


void CBasicStrategy_V2::onUpdateParametersSlot(const QVariantMap& parameters)
{
    this->m_ParametersMap = parameters;
}

void CBasicStrategy_V2::onTimeoutSlot()
{
    // Seed the random generator with a unique seed value
    QRandomGenerator random(QDateTime::currentMSecsSinceEpoch() / 1000);

    // Generate a random float value between 10 and 200
    this->m_genericInfo["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
    QVariantMap tmp = this->m_assetList["SPY"].toMap();
    tmp["pnl"] = random.generateDouble() * (200.0 - 10.0) + 10.0;
    this->m_assetList["SPY"] = tmp;
}

