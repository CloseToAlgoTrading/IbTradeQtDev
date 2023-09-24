
#ifndef CMOMENTUM_H
#define CMOMENTUM_H


#include <QObject>
#include "cbasicstrategy_V2.h"

Q_DECLARE_LOGGING_CATEGORY(MomentumPmLog);

class cMomentum : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit cMomentum(QObject *parent = nullptr);
    virtual ~cMomentum() {};

    virtual bool start() final;
    virtual bool stop() final;

    ModelType modelType() const override { return ModelType::STRATEGY_MOMENTUM; }

public slots:
    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
};

#endif // CMOMENTUM_H
