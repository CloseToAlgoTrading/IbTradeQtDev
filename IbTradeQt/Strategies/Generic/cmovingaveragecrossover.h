#ifndef CMOVINGAVERAGECROSSOVER_H
#define CMOVINGAVERAGECROSSOVER_H

#include "cbasicstrategy.h"
#include "cprocessingbase_v2.h"

Q_DECLARE_LOGGING_CATEGORY(MaCrossoverPmLog);

class CMovingAverageCrossover : public CProcessingBase_v2, public CBasicStrategy
{
    Q_OBJECT
public:
    CMovingAverageCrossover(QObject * parent);

    virtual bool start() final;
    virtual bool stop() final;


public slots:
    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
};

#endif // CMOVINGAVERAGECROSSOVER_H
