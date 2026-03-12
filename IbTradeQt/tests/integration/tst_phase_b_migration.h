#ifndef TST_PHASE_B_MIGRATION_H
#define TST_PHASE_B_MIGRATION_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>

#include "IBComm/OrderRouter.h"
#include "IBComm/AccountRouter.h"
#include "IBComm/PositionRouter.h"
#include "IBComm/HistoricalDataRouter.h"
#include "CHistoricalData.h"
#include "caccountsummary.h"
#include "cposition.h"

using namespace IBDataTypes;

namespace PhaseBHelpers {

inline CAccountSummary convertAccountSummary(const IBComm::AccountSummaryData& data)
{
    CAccountSummary obj;
    obj.setAccount(data.account);
    obj.setAccountType(data.accountType);
    obj.setCurrency(data.currency);
    obj.setBuyingPower(data.buyingPower);
    obj.setTotalCashValue(data.totalCashValue);
    obj.setNetLiquidation(data.netLiquidation);
    obj.setEquityWithLoanValue(data.equityWithLoanValue);
    return obj;
}

inline CPosition convertPosition(const IBComm::PositionUpdate& update)
{
    Contract c;
    c.symbol = update.symbol.toStdString();
    return CPosition(update.account, c, update.quantity, update.avgCost);
}

inline QList<CHistoricalData> convertBars(const QVector<IBComm::HistoricalBar>& bars)
{
    QList<CHistoricalData> histList;
    for (const auto& bar : bars) {
        CHistoricalData hd(0, "", bar.open, bar.high, bar.low, bar.close,
                           static_cast<int>(bar.volume), bar.count, 0.0, 0, false);
        hd.setDateTime(bar.timestamp.toMSecsSinceEpoch());
        histList.append(hd);
    }
    if (!histList.isEmpty())
        histList.last().setIsLast(true);
    return histList;
}

class RouterReceiver : public QObject {
    Q_OBJECT
public:
    int nextValidId = 0;
    CAccountSummary lastSummary;
    QMultiMap<QString, CPosition> positions;
    bool positionSnapshotDone = false;
    QList<CHistoricalData> historicalBars;
    QString historicalSymbol;

public slots:
    void onNextValidId(int id) { nextValidId = id; }

    void onAccountSummary(const IBComm::AccountSummaryData& data) {
        lastSummary = convertAccountSummary(data);
    }

    void onPositionChanged(const IBComm::PositionUpdate& update) {
        positions.insert(update.symbol, convertPosition(update));
    }

    void onPositionSnapshotComplete() {
        positionSnapshotDone = true;
    }

    void onBarsReceived(int /*reqId*/, const QString& symbol,
                        const QVector<IBComm::HistoricalBar>& bars) {
        historicalSymbol = symbol;
        historicalBars = convertBars(bars);
    }
};

} // namespace PhaseBHelpers

class TestPhaseBMigration : public QObject {
    Q_OBJECT

private slots:

    void nextValidId_flowsThroughOrderRouter()
    {
        IBComm::OrderRouter router;
        PhaseBHelpers::RouterReceiver receiver;

        connect(&router, &IBComm::OrderRouter::nextValidIdReceived,
                &receiver, &PhaseBHelpers::RouterReceiver::onNextValidId);

        router.onNextValidId(42);

        QCOMPARE(receiver.nextValidId, 42);
    }

    void accountSummary_convertsCorrectly()
    {
        IBComm::AccountRouter router;
        PhaseBHelpers::RouterReceiver receiver;

        connect(&router, &IBComm::AccountRouter::accountSummaryUpdated,
                &receiver, &PhaseBHelpers::RouterReceiver::onAccountSummary);

        router.onAccountSummary("DU12345", "AccountType", "INDIVIDUAL", "USD");
        router.onAccountSummary("DU12345", "BuyingPower", "50000.0", "USD");
        router.onAccountSummary("DU12345", "TotalCashValue", "25000.0", "USD");
        router.onAccountSummary("DU12345", "NetLiquidation", "100000.0", "USD");
        router.onAccountSummary("DU12345", "EquityWithLoanValue", "120000.0", "USD");
        router.onAccountSummaryEnd(1);

        QCOMPARE(receiver.lastSummary.getAccount(), QString("DU12345"));
        QCOMPARE(receiver.lastSummary.getAccountType(), QString("INDIVIDUAL"));
        QCOMPARE(receiver.lastSummary.getCurrency(), QString("USD"));
        QCOMPARE(receiver.lastSummary.getBuyingPower(), 50000.0);
        QCOMPARE(receiver.lastSummary.getTotalCashValue(), 25000.0);
        QCOMPARE(receiver.lastSummary.getNetLiquidation(), 100000.0);
        QCOMPARE(receiver.lastSummary.getEquityWithLoanValue(), 120000.0);
    }

    void positions_flowThroughPositionRouter()
    {
        IBComm::PositionRouter router;
        PhaseBHelpers::RouterReceiver receiver;

        connect(&router, &IBComm::PositionRouter::positionChanged,
                &receiver, &PhaseBHelpers::RouterReceiver::onPositionChanged);
        connect(&router, &IBComm::PositionRouter::positionSnapshotComplete,
                &receiver, &PhaseBHelpers::RouterReceiver::onPositionSnapshotComplete);

        router.onPosition("DU12345", "AAPL", 100.0, 150.25);
        router.onPosition("DU12345", "MSFT", 50.0, 380.50);
        router.onPositionEnd();

        QVERIFY(receiver.positions.contains("AAPL"));
        QVERIFY(receiver.positions.contains("MSFT"));
        QCOMPARE(receiver.positions.value("AAPL").getPos(), 100.0);
        QCOMPARE(receiver.positions.value("AAPL").getAvgCost(), 150.25);
        QCOMPARE(receiver.positions.value("MSFT").getPos(), 50.0);
        QCOMPARE(receiver.positions.value("MSFT").getAvgCost(), 380.50);
        QVERIFY(receiver.positionSnapshotDone);
    }

    void positions_convertContractSymbol()
    {
        IBComm::PositionUpdate update;
        update.account = "ACC1";
        update.symbol = "TSLA";
        update.quantity = 25.0;
        update.avgCost = 250.0;

        CPosition pos = PhaseBHelpers::convertPosition(update);
        QCOMPARE(QString::fromStdString(pos.getContract().symbol), QString("TSLA"));
        QCOMPARE(pos.getAccount(), QString("ACC1"));
        QCOMPARE(pos.getPos(), 25.0);
        QCOMPARE(pos.getAvgCost(), 250.0);
    }

    void historicalData_flowsThroughRouter()
    {
        IBComm::HistoricalDataRouter router;
        PhaseBHelpers::RouterReceiver receiver;

        connect(&router, &IBComm::HistoricalDataRouter::barsReceived,
                &receiver, &PhaseBHelpers::RouterReceiver::onBarsReceived);

        router.setReqIdSymbol(100, "AAPL");
        router.onHistoricalBar(100, "20250101", 150.0, 155.0, 148.0, 153.0, 1000.0, 50);
        router.onHistoricalBar(100, "20250102", 153.0, 158.0, 151.0, 157.0, 1200.0, 60);
        router.onHistoricalDataEnd(100);

        QCOMPARE(receiver.historicalSymbol, QString("AAPL"));
        QCOMPARE(receiver.historicalBars.size(), 2);
        QCOMPARE(receiver.historicalBars[0].getOpen(), 150.0);
        QCOMPARE(receiver.historicalBars[0].getHigh(), 155.0);
        QCOMPARE(receiver.historicalBars[0].getLow(), 148.0);
        QCOMPARE(receiver.historicalBars[0].getClose(), 153.0);
        QCOMPARE(receiver.historicalBars[1].getOpen(), 153.0);
        QCOMPARE(receiver.historicalBars[1].getClose(), 157.0);
        QVERIFY(receiver.historicalBars[1].getIsLast());
        QVERIFY(!receiver.historicalBars[0].getIsLast());
    }

    void historicalData_conversionPreservesFields()
    {
        IBComm::HistoricalBar bar;
        bar.symbol = "TEST";
        bar.open = 10.0;
        bar.high = 12.0;
        bar.low = 9.0;
        bar.close = 11.5;
        bar.volume = 500.0;
        bar.count = 30;
        bar.timestamp = QDateTime(QDate(2025, 6, 15), QTime(10, 30, 0));

        QVector<IBComm::HistoricalBar> bars = { bar };
        auto result = PhaseBHelpers::convertBars(bars);

        QCOMPARE(result.size(), 1);
        QCOMPARE(result[0].getOpen(), 10.0);
        QCOMPARE(result[0].getHigh(), 12.0);
        QCOMPARE(result[0].getLow(), 9.0);
        QCOMPARE(result[0].getClose(), 11.5);
        QCOMPARE(result[0].getVolume(), 500.0);
        QCOMPARE(result[0].getCount(), 30);
        QVERIFY(result[0].getIsLast());
    }
};

#endif // TST_PHASE_B_MIGRATION_H
