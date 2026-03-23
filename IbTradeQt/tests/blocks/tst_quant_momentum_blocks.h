#ifndef TST_QUANT_MOMENTUM_BLOCKS_H
#define TST_QUANT_MOMENTUM_BLOCKS_H

#include <QObject>
#include <QtTest>

class TestQuantMomentumBlocks : public QObject {
    Q_OBJECT
private slots:
    void momentumSemantic_usesPeriodBarReturnAndTopN();
    void simpleRebalance_equalWeight_usesAllocatedCapital();
    void simpleRebalance_rotation_emitsExitForDroppedHoldings();
    void maxPositionRisk_stopLoss_emitsSellOnce();
};

#endif
