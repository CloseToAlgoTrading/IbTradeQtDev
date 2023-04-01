#include "cstrategyfactory.h"

#include "cteststrategy.h"
#include "AutoDeltAlignmentProcessing.h"

#include <QSharedPointer>

ptrGenericModelType CStrategyFactory::createNewStrategy(STRATEGY_ID id)
{
    ptrGenericModelType ret = nullptr;
    switch (id) {
    case STRATEGY_BASIC_TEST:
    {
        ret = QSharedPointer<CTestStrategy>::create();
        break;
    }
    case STRATEGY_AUTO_DELTA:
    {
        ret = QSharedPointer<autoDeltaAligPM>::create(nullptr);
 //       ret = QSharedPointer<CTestStrategy>::create();
        break;
    }
    default:
        break;
    }

    return ret;
}
