#include "cbasicstrategy.h"

CBasicStrategy::CBasicStrategy():
      m_Models()
    , m_ParametersMap()
    , m_InfoMap()
    , m_Name()
    , m_DataProvider()
    , m_assetList()
    , m_genericInfo()
{
    this->m_genericInfo["pnl"] = 95.2f;
    this->m_genericInfo["pnlpnc"] = "2.1 %";


//    this->m_assetList["SPY"] = QVariantMap({{"pnl",23.0f}, {"aprice",100.0f}});
//    this->m_assetList["V"] = QVariantMap({{"pnl",2.0f}, {"aprice",320.1f}});
}

void CBasicStrategy::addModel(ptrGenericModelType pModel)
{
    this->m_Models.append(pModel);
}

void CBasicStrategy::removeModel(ptrGenericModelType pModel)
{
    if (pModel) {
        this->m_Models.removeOne(pModel);
    }
}

QList<ptrGenericModelType>& CBasicStrategy::getModels()
{
    return this->m_Models;
}

QString CBasicStrategy::getName()
{
    return this->m_InfoMap["name"].toString();
}

void CBasicStrategy::setName(const QString& name)
{
    this->m_InfoMap["name"] = name;
}

void CBasicStrategy::setParameters(const QVariantMap &parametersMap)
{
    this->m_ParametersMap = parametersMap;
    //emit onUpdateParametersSignal(this->m_ParametersMap);
}

const QVariantMap &CBasicStrategy::getParameters()
{
    return this->m_ParametersMap;
}

bool CBasicStrategy::start()
{
    this->m_InfoMap["IsStarted"] = true;
    return true;
}

bool CBasicStrategy::stop()
{
    this->m_InfoMap["IsStarted"] = false;
    return false;
}


QVariantMap CBasicStrategy::assetList() const
{
    return m_assetList;
}

void CBasicStrategy::setAssetList(const QVariantMap &newAssetList)
{
    m_assetList = newAssetList;
}

QVariantMap CBasicStrategy::genericInfo() const
{
    return m_genericInfo;
}

void CBasicStrategy::setGenericInfo(const QVariantMap &newGenericInfo)
{
    m_genericInfo = newGenericInfo;
}


void CBasicStrategy::onUpdateParametersSlot(const QVariantMap& parameters)
{
    this->m_ParametersMap = parameters;
}

