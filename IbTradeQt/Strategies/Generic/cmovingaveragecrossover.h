#ifndef CMOVINGAVERAGECROSSOVER_H
#define CMOVINGAVERAGECROSSOVER_H

#include "cbasicstrategy_v2.h"

Q_DECLARE_LOGGING_CATEGORY(MaCrossoverPmLog);

class CMovingAverageCrossover : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CMovingAverageCrossover(QObject * parent = nullptr);
    virtual ~CMovingAverageCrossover() {};

    virtual bool start() final;
    virtual bool stop() final;

    ModelType modelType() const override { return ModelType::STRATEGY_MA; }


public slots:
    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
};

#endif // CMOVINGAVERAGECROSSOVER_H
