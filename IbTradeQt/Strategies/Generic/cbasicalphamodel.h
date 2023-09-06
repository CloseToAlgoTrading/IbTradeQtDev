
#ifndef CBASICALPHAMODEL_H
#define CBASICALPHAMODEL_H

#include "cbasicstrategy_v2.h"

Q_DECLARE_LOGGING_CATEGORY(BasicAlphaModelLog);

class CBasicAlphaModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicAlphaModel(QObject *parent = nullptr);
    virtual ~CBasicAlphaModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_ALPHA_MODEL; }
};

#endif // CBASICALPHAMODEL_H
