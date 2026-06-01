#ifndef TST_TRADING_READINESS_H
#define TST_TRADING_READINESS_H

#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>

#include "TradingReadiness.h"

class TestTradingReadiness : public QObject
{
    Q_OBJECT

private:
    static QJsonObject staticPipeline()
    {
        QJsonObject block;
        block.insert(QStringLiteral("blockId"), QStringLiteral("static-list-selection"));
        block.insert(QStringLiteral("config"),
                     QJsonObject{{QStringLiteral("symbols"),
                                  QJsonArray{QStringLiteral("NVDA"), QStringLiteral("MSFT")}}});
        return QJsonObject{{QStringLiteral("selection"), QJsonArray{block}}};
    }

private slots:
    void liveModeBlocksWhenBrokerDisconnected()
    {
        const auto state = TradingUX::computeLiveReadiness(staticPipeline(),
                                                           true,
                                                           false,
                                                           false,
                                                           false,
                                                           true);

        QVERIFY(state.hasBlocks());
        QVERIFY(state.blockingMessages.contains(QStringLiteral("Connect IB/TWS before live orders")));
        QCOMPARE(state.resolvedSymbolCount, 2);
    }

    void dryRunRemainsAvailableWithoutBroker()
    {
        const auto state = TradingUX::computeLiveReadiness(staticPipeline(),
                                                           false,
                                                           false,
                                                           false,
                                                           false,
                                                           false);

        QVERIFY(!state.hasBlocks());
        QVERIFY(state.warningMessages.contains(QStringLiteral("Dry run: orders are simulated")));
    }

    void ibBacktestWarnsWhenConnectionNeeded()
    {
        const auto state = TradingUX::computeBacktestReadiness({QStringLiteral("NVDA")},
                                                               QStringLiteral("ib"),
                                                               false,
                                                               false,
                                                               true);

        QVERIFY(!state.hasBlocks());
        QVERIFY(state.warningMessages.contains(
            QStringLiteral("Requires active IB/TWS connection for missing data")));
    }

    void ibBacktestCachedCoverageCanRunDisconnected()
    {
        const auto state = TradingUX::computeBacktestReadiness({QStringLiteral("NVDA")},
                                                               QStringLiteral("ib"),
                                                               false,
                                                               true,
                                                               false);

        QVERIFY(!state.hasBlocks());
        QVERIFY(state.warningMessages.contains(
            QStringLiteral("IB/TWS disconnected; cached data can still run")));
    }
};

#endif // TST_TRADING_READINESS_H
