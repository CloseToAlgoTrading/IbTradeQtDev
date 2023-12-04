
#ifndef CBASICEXECUTIONMODEL_H
#define CBASICEXECUTIONMODEL_H

#include "cbasicstrategy_V2.h"

Q_DECLARE_LOGGING_CATEGORY(BasicExecutionModelLog);

class CBasicExecutionModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicExecutionModel(QObject *parent = nullptr);
    virtual ~CBasicExecutionModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_EXECTION_MODEL; }
public slots:
    virtual void processData(DataListPtr data) override;
};

#endif // CBASICEXECUTIONMODEL_H
