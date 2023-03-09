#include "caccount.h"

CAccount::CAccount():
    mParametersMap()
{
    initParametersMap();
}

void CAccount::initParametersMap()
{
    this->mParametersMap["slowMA"] = 25;
    this->mParametersMap["fastMA"] = 5;
}

void CAccount::setParametersMap(const QVariantMap &newParametersMap)
{
    mParametersMap = newParametersMap;
}

QVariantMap CAccount::parametersMap() const
{
    return mParametersMap;
}

