#include "cstrategyfactory.h"

#include "cteststrategy.h"
#include "cmovingaveragecrossover.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cbasicroot.h"
#include "cbasicstrategy_V2.h"
#include <QSharedPointer>

ptrGenericModelType CStrategyFactory::createNewStrategy(ModelType id)
{
    switch (id) {
    case ModelType::ROOT:
        return QSharedPointer<CBasicRoot>::create();
    case ModelType::ACCOUNT:
        return QSharedPointer<CBasicAccount>::create();
    case ModelType::PORTFOLIO:
        return QSharedPointer<CBasicPortfolio>::create();
    case ModelType::STRATEGY:
        return QSharedPointer<CBasicStrategy_V2>::create();
    case ModelType::STRATEGY_BASIC_TEST:
        return QSharedPointer<CTestStrategy>::create();
    case ModelType::STRATEGY_MA:
        return QSharedPointer<CMovingAverageCrossover>::create();
    default:
        return nullptr;
    }
}
