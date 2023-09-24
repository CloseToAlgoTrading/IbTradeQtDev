
#ifndef CBASEREBALANCEMODEL_H
#define CBASEREBALANCEMODEL_H

#include "cbasicstrategy_v2.h"

Q_DECLARE_LOGGING_CATEGORY(BasicRebalanceModelLog);

class CBaseRebalanceModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBaseRebalanceModel(QObject *parent = nullptr);
    virtual ~CBaseRebalanceModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_REBALANCE_MODEL; }
};

#endif // CBASEREBALANCEMODEL_H
