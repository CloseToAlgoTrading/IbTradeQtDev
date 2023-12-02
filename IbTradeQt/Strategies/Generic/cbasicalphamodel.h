
#ifndef CBASICALPHAMODEL_H
#define CBASICALPHAMODEL_H

#include "cbasicstrategy_V2.h"
#include "UnifiedModelData.h"
#include <QStringList>

Q_DECLARE_LOGGING_CATEGORY(BasicAlphaModelLog);

class CBasicAlphaModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicAlphaModel(QObject *parent = nullptr);
    virtual ~CBasicAlphaModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_ALPHA_MODEL; }

public slots:
    virtual void processData(DataListPtr data) override;
    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);

};

#endif // CBASICALPHAMODEL_H
