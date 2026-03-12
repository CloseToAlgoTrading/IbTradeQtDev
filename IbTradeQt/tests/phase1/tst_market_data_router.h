#ifndef TST_MARKET_DATA_ROUTER_H
#define TST_MARKET_DATA_ROUTER_H

#include <QtTest>
#include <QSignalSpy>
#include "IBComm/MarketDataRouter.h"

class TestMarketDataRouter : public QObject
{
    Q_OBJECT

private slots:
    void tickEmittedOnPriceUpdate()
    {
        IBComm::MarketDataRouter router;
        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tick);

        router.onTickPrice(1, "AAPL", 149.50, 150.00);

        QCOMPARE(spy.count(), 1);
        auto tick = spy.at(0).at(0).value<IBComm::MarketTick>();
        QCOMPARE(tick.symbol, QString("AAPL"));
        QCOMPARE(tick.bid, 149.50);
        QCOMPARE(tick.ask, 150.00);
        QCOMPARE(tick.reqId, 1);
        QVERIFY(tick.timestamp.isValid());
    }

    void midPriceCalculation()
    {
        IBComm::MarketTick tick;
        tick.bid = 100.0;
        tick.ask = 102.0;
        QCOMPARE(tick.mid(), 101.0);

        tick.bid = 0.0;
        tick.ask = 0.0;
        QCOMPARE(tick.mid(), 0.0);
    }

    void lastPriceCache()
    {
        IBComm::MarketDataRouter router;

        router.onTickPrice(1, "AAPL", 149.0, 150.0);
        router.onTickPrice(2, "MSFT", 300.0, 301.0);

        auto aaplTick = router.lastPrice("AAPL");
        QCOMPARE(aaplTick.symbol, QString("AAPL"));
        QCOMPARE(aaplTick.bid, 149.0);

        auto msftTick = router.lastPrice("MSFT");
        QCOMPARE(msftTick.symbol, QString("MSFT"));
        QCOMPARE(msftTick.bid, 300.0);
    }

    void lastPriceCacheUpdatesOnNewTick()
    {
        IBComm::MarketDataRouter router;

        router.onTickPrice(1, "AAPL", 149.0, 150.0);
        router.onTickPrice(1, "AAPL", 151.0, 152.0);

        auto tick = router.lastPrice("AAPL");
        QCOMPARE(tick.bid, 151.0);
        QCOMPARE(tick.ask, 152.0);
    }

    void lastPriceForUnknownSymbol()
    {
        IBComm::MarketDataRouter router;
        auto tick = router.lastPrice("UNKNOWN");
        QCOMPARE(tick.bid, 0.0);
        QCOMPARE(tick.ask, 0.0);
        QVERIFY(tick.symbol.isEmpty());
    }

    void barCloseEmitted()
    {
        IBComm::MarketDataRouter router;
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::barClose);

        QDateTime now = QDateTime::currentDateTimeUtc();
        router.onBarComplete(1, "AAPL", now);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("AAPL"));
        QCOMPARE(spy.at(0).at(1).toDateTime(), now);
    }

    void multipleSymbolsMultipleTicks()
    {
        IBComm::MarketDataRouter router;
        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tick);

        router.onTickPrice(1, "AAPL", 149.0, 150.0);
        router.onTickPrice(2, "MSFT", 300.0, 301.0);
        router.onTickPrice(3, "GOOG", 2800.0, 2801.0);

        QCOMPARE(spy.count(), 3);

        auto tick1 = spy.at(0).at(0).value<IBComm::MarketTick>();
        auto tick2 = spy.at(1).at(0).value<IBComm::MarketTick>();
        auto tick3 = spy.at(2).at(0).value<IBComm::MarketTick>();
        QCOMPARE(tick1.symbol, QString("AAPL"));
        QCOMPARE(tick2.symbol, QString("MSFT"));
        QCOMPARE(tick3.symbol, QString("GOOG"));
    }

    void tickTimestampIsSet()
    {
        IBComm::MarketDataRouter router;
        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&router, &IBComm::MarketDataRouter::tick);

        QDateTime before = QDateTime::currentDateTime();
        router.onTickPrice(1, "AAPL", 149.0, 150.0);
        QDateTime after = QDateTime::currentDateTime();

        auto tick = spy.at(0).at(0).value<IBComm::MarketTick>();
        QVERIFY(tick.timestamp >= before);
        QVERIFY(tick.timestamp <= after);
    }
};

#endif // TST_MARKET_DATA_ROUTER_H
