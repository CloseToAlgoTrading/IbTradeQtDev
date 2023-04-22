#ifndef CSTRATEGYFACTORY_H
#define CSTRATEGYFACTORY_H

#include "cgenericmodelApi.h"
#include "ModelType.h"

class CStrategyFactory
{

public:
    static ptrGenericModelType createNewStrategy(ModelType id);
};

#endif // CSTRATEGYFACTORY_H
