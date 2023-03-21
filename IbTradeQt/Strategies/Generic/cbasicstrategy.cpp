#include "cbasicstrategy.h"

CBasicStrategy::CBasicStrategy():
    m_Models()
  , m_ParametersMap()
  , m_InfoMap()
  , m_Name()
{
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

void CBasicStrategy::onUpdateParametersSlot(const QVariantMap& parameters)
{
    this->m_ParametersMap = parameters;
}

