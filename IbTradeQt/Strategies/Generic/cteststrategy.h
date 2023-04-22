#ifndef CTESTSTRATEGY_H
#define CTESTSTRATEGY_H

#include "cbasicstrategy_V2.h"

class CTestStrategy : public CBasicStrategy_V2
{
public:
    explicit CTestStrategy(QObject *parent = nullptr);
    virtual ~CTestStrategy() {};

    ModelType modelType() const override { return ModelType::STRATEGY_BASIC_TEST; }
};

#endif // CTESTSTRATEGY_H
