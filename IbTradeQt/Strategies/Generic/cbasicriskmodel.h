
#ifndef CBASICRISKMODEL_H
#define CBASICRISKMODEL_H

#include "cbasicstrategy_V2.h"

Q_DECLARE_LOGGING_CATEGORY(BasicRiskModelLog);

class CBasicRiskModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicRiskModel(QObject *parent = nullptr);
    virtual ~CBasicRiskModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_RISK_MODEL; }

public slots:
    virtual void processData(DataListPtr data) override;
};

#endif // CBASICRISKMODEL_H
