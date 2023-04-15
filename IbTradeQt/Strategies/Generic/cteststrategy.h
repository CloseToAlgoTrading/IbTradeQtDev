#ifndef CTESTSTRATEGY_H
#define CTESTSTRATEGY_H

#include "cbasicstrategy_V2.h"

class CTestStrategy : public CBasicStrategy_V2
{
public:
    explicit CTestStrategy(QObject *parent = nullptr);
    virtual ~CTestStrategy() {};
};

#endif // CTESTSTRATEGY_H
