#include "ProcessingRouterSink.h"
#include "cprocessingbase_v2.h"
#include "cbrokerdataprovider.h"
#include "CHistoricalData.h"
#include "caccountsummary.h"
#include "cposition.h"
#include "cexecutionreport.h"
#include "ccommissionreport.h"
#include "GlobalDef.h"

using namespace IBDataTypes;

namespace IBComm {

ProcessingRouterSink::ProcessingRouterSink(CProcessingBase_v2* base, QObject* parent)
    : QObject(parent)
    , m_base(base)
{}

ProcessingRouterSink::~ProcessingRouterSink()
{
    unbind();
}

void ProcessingRouterSink::bindTo(QSharedPointer<CBrokerDataProvider> client)
{
    unbind();
    if (!m_base || !client)
        return;

    m_bound = client.data();

    if (auto* r = client->orderRouter()) {
        connect(r, &OrderRouter::nextValidIdReceived,
                this, &ProcessingRouterSink::slotRouterNextValidId, Qt::QueuedConnection);
        connect(r, &OrderRouter::executionReceived,
                this, &ProcessingRouterSink::slotRouterExecution, Qt::QueuedConnection);
        connect(r, &OrderRouter::commissionReceived,
                this, &ProcessingRouterSink::slotRouterCommission, Qt::QueuedConnection);
    }
    if (auto* r = client->accountRouter()) {
        connect(r, &AccountRouter::accountSummaryUpdated,
                this, &ProcessingRouterSink::slotRouterAccountSummary, Qt::QueuedConnection);
    }
    if (auto* r = client->positionRouter()) {
        connect(r, &PositionRouter::positionChanged,
                this, &ProcessingRouterSink::slotRouterPositionChanged, Qt::QueuedConnection);
        connect(r, &PositionRouter::positionSnapshotComplete,
                this, &ProcessingRouterSink::slotRouterPositionSnapshotComplete, Qt::QueuedConnection);
    }
    if (auto* r = client->historicalDataRouter()) {
        connect(r, &HistoricalDataRouter::barsReceived,
                this, &ProcessingRouterSink::slotRouterBarsReceived, Qt::QueuedConnection);
    }
}

void ProcessingRouterSink::unbind()
{
    if (!m_bound)
        return;

    if (auto* r = m_bound->orderRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_bound->accountRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_bound->positionRouter())
        disconnect(r, nullptr, this, nullptr);
    if (auto* r = m_bound->historicalDataRouter())
        disconnect(r, nullptr, this, nullptr);

    m_bound = nullptr;
}

void ProcessingRouterSink::slotRouterNextValidId(int orderId)
{
    if (m_base)
        m_base->setNextValidId(static_cast<qint32>(orderId));
}

void ProcessingRouterSink::slotRouterAccountSummary(const AccountSummaryData& data)
{
    if (!m_base)
        return;
    CAccountSummary obj;
    obj.setAccount(data.account);
    obj.setAccountType(data.accountType);
    obj.setCurrency(data.currency);
    obj.setBuyingPower(data.buyingPower);
    obj.setTotalCashValue(data.totalCashValue);
    obj.setNetLiquidation(data.netLiquidation);
    obj.setEquityWithLoanValue(data.equityWithLoanValue);
    emit m_base->signalRecvAccountSummary(obj);
}

void ProcessingRouterSink::slotRouterPositionChanged(const PositionUpdate& update)
{
    if (!m_base)
        return;
    Contract c;
    c.symbol = update.symbol.toStdString();
    CPosition pos(update.account, c, update.quantity, update.avgCost);
    m_base->m_positionMap.insert(update.symbol, pos);
}

void ProcessingRouterSink::slotRouterPositionSnapshotComplete()
{
    if (m_base)
        emit m_base->signalEndRecvPosition();
}

void ProcessingRouterSink::slotRouterBarsReceived(int requestId, const QString& symbol,
                                                  const QVector<HistoricalBar>& bars)
{
    Q_UNUSED(requestId)
    if (!m_base)
        return;
    QList<CHistoricalData> histList;
    for (const auto& bar : bars) {
        CHistoricalData hd(0, "", bar.open, bar.high, bar.low, bar.close,
                           static_cast<int>(bar.volume), bar.count, 0.0, 0, false);
        hd.setDateTime(bar.timestamp.toMSecsSinceEpoch());
        histList.append(hd);
    }
    if (!histList.isEmpty()) {
        histList.last().setIsLast(true);
    }
    emit m_base->signalCbkRecvHistoricalData(histList, symbol);
}

void ProcessingRouterSink::slotRouterExecution(const ExecutionReport& report)
{
    if (!m_base)
        return;
    CExecutionReport obj(report.orderId, report.symbol, report.avgPrice, report.shares, report.execId);
    emit m_base->signalRecvExecutionReport(obj);
}

void ProcessingRouterSink::slotRouterCommission(const CommissionUpdate& update)
{
    if (!m_base)
        return;
    CCommissionReport obj(update.execId, update.commission, update.currency, update.realizedPnL, 0.0, 0);
    emit m_base->signalRecvCommissionReport(obj);
}

} // namespace IBComm
