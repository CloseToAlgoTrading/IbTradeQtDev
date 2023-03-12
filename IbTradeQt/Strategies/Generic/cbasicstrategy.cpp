#include "cbasicstrategy.h"

CBasicStrategy::CBasicStrategy():
    m_Models()
  , m_ParametersMap()
  , m_InfoMap()
  , m_Name()
{
}

bool CBasicStrategy::addModel(ptrGenericModelType pModel)
{
    this->m_Models.append(pModel);
    return true;
}

bool CBasicStrategy::removeModel(ptrGenericModelType pModel)
{
    this->m_Models.removeOne(pModel);
    return true;
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

bool CBasicStrategy::start()
{
    return true;
}

bool CBasicStrategy::stop()
{
    return true;
}

void CBasicStrategy::setParameters(const QVariantMap &parametersMap)
{
    this->m_ParametersMap = parametersMap;
}

const QVariantMap &CBasicStrategy::getParameters()
{
    return this->m_ParametersMap;
}

