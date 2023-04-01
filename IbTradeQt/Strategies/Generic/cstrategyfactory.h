#ifndef CSTRATEGYFACTORY_H
#define CSTRATEGYFACTORY_H

#include "cgenericmodelApi.h"

enum STRATEGY_ID {
    STRATEGY_NONE = 0,
    STRATEGY_BASIC_TEST,
    STRATEGY_AUTO_DELTA,
    STRATEGY_MA_CROSS
};

class CStrategyFactory
{

public:
    static ptrGenericModelType createNewStrategy(STRATEGY_ID id);
};

#endif // CSTRATEGYFACTORY_H