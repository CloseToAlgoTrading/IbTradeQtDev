#ifndef TST_PHASE_E_REMAINING_ROUTERS_H
#define TST_PHASE_E_REMAINING_ROUTERS_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>

#include "IBComm/MarketDataRouter.h"
#include "IBComm/MarketDepthRouter.h"
#include "IBComm/HistoricalDataRouter.h"

class TestPhaseE_RemainingRouters : public QObject {
    Q_OBJECT

private slots:

    void tickGeneric_flowsThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        router.registerReqIdSymbol(10, "AAPL");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickGenericReceived);

        router.onTickGeneric(10, 49, 0.35);

        QCOMPARE(spy.count(), 1);
        auto gt = spy.at(0).at(0).value<IBComm::GenericTick>();
        QCOMPARE(gt.symbol, QString("AAPL"));
        QCOMPARE(gt.tickType, 49);
        QCOMPARE(gt.value, 0.35);
    }

    void tickString_flowsThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        router.registerReqIdSymbol(11, "MSFT");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickStringReceived);

        router.onTickString(11, 45, "1709568000");

        QCOMPARE(spy.count(), 1);
        auto st = spy.at(0).at(0).value<IBComm::StringTick>();
        QCOMPARE(st.symbol, QString("MSFT"));
        QCOMPARE(st.tickType, 45);
        QCOMPARE(st.value, QString("1709568000"));
    }

    void optionComputation_flowsThroughMarketDataRouter()
    {
        IBComm::MarketDataRouter router;
        router.registerReqIdSymbol(12, "SPY");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::optionComputationReceived);

        router.onTickOptionComputation(12, 10, 0.25, 0.5, 3.50, 0.10, 0.02, 0.15, -0.03, 450.0);

        QCOMPARE(spy.count(), 1);
        auto oc = spy.at(0).at(0).value<IBComm::OptionComputation>();
        QCOMPARE(oc.symbol, QString("SPY"));
        QCOMPARE(oc.tickType, 10);
        QCOMPARE(oc.impliedVol, 0.25);
        QCOMPARE(oc.delta, 0.5);
        QCOMPARE(oc.optPrice, 3.50);
        QCOMPARE(oc.pvDividend, 0.10);
        QCOMPARE(oc.gamma, 0.02);
        QCOMPARE(oc.vega, 0.15);
        QCOMPARE(oc.theta, -0.03);
        QCOMPARE(oc.undPrice, 450.0);
    }

    void depthUpdate_flowsThroughMarketDepthRouter()
    {
        IBComm::MarketDepthRouter router;
        router.registerReqIdSymbol(20, "AAPL");
        QSignalSpy spy(&router, &IBComm::MarketDepthRouter::depthUpdated);

        router.onDepthUpdate(20, 0, 0, 1, 150.25, 100.0);

        QCOMPARE(spy.count(), 1);
        auto d = spy.at(0).at(0).value<IBComm::DepthUpdate>();
        QCOMPARE(d.symbol, QString("AAPL"));
        QCOMPARE(d.position, 0);
        QCOMPARE(d.operation, 0);
        QCOMPARE(d.side, 1);
        QCOMPARE(d.price, 150.25);
        QCOMPARE(d.size, 100.0);
    }

    void depthL2Update_flowsThroughMarketDepthRouter()
    {
        IBComm::MarketDepthRouter router;
        router.registerReqIdSymbol(21, "GOOG");
        QSignalSpy spy(&router, &IBComm::MarketDepthRouter::depthL2Updated);

        router.onDepthL2Update(21, 1, "ARCX", 1, 0, 175.50, 200.0, true);

        QCOMPARE(spy.count(), 1);
        auto d = spy.at(0).at(0).value<IBComm::DepthL2Update>();
        QCOMPARE(d.symbol, QString("GOOG"));
        QCOMPARE(d.position, 1);
        QCOMPARE(d.marketMaker, QString("ARCX"));
        QCOMPARE(d.operation, 1);
        QCOMPARE(d.side, 0);
        QCOMPARE(d.price, 175.50);
        QCOMPARE(d.size, 200.0);
        QVERIFY(d.isSmartDepth);
    }

    void historicalTicksLast_flowsThroughHistoricalDataRouter()
    {
        IBComm::HistoricalDataRouter router;
        router.setReqIdSymbol(30, "TSLA");
        QSignalSpy spy(&router, &IBComm::HistoricalDataRouter::historicalTicksLastReceived);

        QVector<IBComm::HistoricalTickLast> ticks;
        IBComm::HistoricalTickLast t1;
        t1.price = 250.0;
        t1.size = 10.0;
        t1.time = 1709568000;
        t1.exchange = "ARCA";
        t1.specialConditions = "";
        ticks.append(t1);

        IBComm::HistoricalTickLast t2;
        t2.price = 251.0;
        t2.size = 5.0;
        t2.time = 1709568001;
        t2.exchange = "NYSE";
        t2.specialConditions = "T";
        ticks.append(t2);

        router.onHistoricalTicksLast(30, ticks, true);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 30);
        QCOMPARE(spy.at(0).at(1).toString(), QString("TSLA"));
        auto received = spy.at(0).at(2).value<QVector<IBComm::HistoricalTickLast>>();
        QCOMPARE(received.size(), 2);
        QCOMPARE(received[0].price, 250.0);
        QCOMPARE(received[0].exchange, QString("ARCA"));
        QCOMPARE(received[1].price, 251.0);
        QCOMPARE(received[1].specialConditions, QString("T"));
        QCOMPARE(spy.at(0).at(3).toBool(), true);
    }

    void tickGeneric_unknownReqId_usesReqIdAsSymbol()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tickGenericReceived);

        router.onTickGeneric(999, 49, 1.0);

        QCOMPARE(spy.count(), 1);
        auto gt = spy.at(0).at(0).value<IBComm::GenericTick>();
        QCOMPARE(gt.symbol, QString("999"));
    }
};

#endif // TST_PHASE_E_REMAINING_ROUTERS_H
