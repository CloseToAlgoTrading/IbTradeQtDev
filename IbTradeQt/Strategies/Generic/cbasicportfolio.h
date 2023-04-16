#ifndef CBASICPORTFOLIO_H
#define CBASICPORTFOLIO_H

#include "cbasicstrategy_V2.h"

class CBasicPortfolio : public CBasicStrategy_V2
{
public:
    explicit CBasicPortfolio(QObject *parent = nullptr);
    virtual ~CBasicPortfolio() {};

    ModelType modelType() const override { return ModelType::PORTFOLIO; }
};

#endif // CBASICPORTFOLIO_H
