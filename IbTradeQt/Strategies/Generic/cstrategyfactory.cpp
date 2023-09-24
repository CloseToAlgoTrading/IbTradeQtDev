#include "cstrategyfactory.h"

#include "cbasicexecutionmodel.h"
#include "cteststrategy.h"
#include "cmovingaveragecrossover.h"
#include "cbasicaccount.h"
#include "cbasicportfolio.h"
#include "cbasicroot.h"
#include "cbasicstrategy_V2.h"
#include "cmomentum.h"
#include "cbasicselectionmodel.h"
#include "cbasicalphamodel.h"
#include "cbaserebalancemodel.h"
#include "cbasicriskmodel.h"
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
    case ModelType::STRATEGY_MOMENTUM:
        return QSharedPointer<cMomentum>::create();
    case ModelType::STRATEGY_SELECTION_MODEL:
        return QSharedPointer<CBasicSelectionModel>::create();
    case ModelType::STRATEGY_ALPHA_MODEL:
        return QSharedPointer<CBasicAlphaModel>::create();
    case ModelType::STRATEGY_REBALANCE_MODEL:
        return QSharedPointer<CBaseRebalanceModel>::create();
    case ModelType::STRATEGY_RISK_MODEL:
        return QSharedPointer<CBasicRiskModel>::create();
    case ModelType::STRATEGY_EXECTION_MODEL:
        return QSharedPointer<CBasicExecutionModel>::create();
    default:
        return nullptr;
    }
}
