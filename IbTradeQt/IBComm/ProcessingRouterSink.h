#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include "OrderRouter.h"
#include "AccountRouter.h"
#include "PositionRouter.h"
#include "HistoricalDataRouter.h"

class CBrokerDataProvider;
class CProcessingBase_v2;

namespace IBComm {

/// Connects typed IB routers to CProcessingBase_v2 without pulling router headers into Common/.
class ProcessingRouterSink : public QObject {
    Q_OBJECT
public:
    explicit ProcessingRouterSink(CProcessingBase_v2* base, QObject* parent = nullptr);
    ~ProcessingRouterSink() override;

    void bindTo(QSharedPointer<CBrokerDataProvider> client);
    void unbind();

private slots:
    void slotRouterNextValidId(int orderId);
    void slotRouterAccountSummary(const IBComm::AccountSummaryData& data);
    void slotRouterPositionChanged(const IBComm::PositionUpdate& update);
    void slotRouterPositionSnapshotComplete();
    void slotRouterBarsReceived(int requestId, const QString& symbol,
                                const QVector<IBComm::HistoricalBar>& bars);
    void slotRouterExecution(const IBComm::ExecutionReport& report);
    void slotRouterCommission(const IBComm::CommissionUpdate& update);

private:
    CProcessingBase_v2* m_base = nullptr;
    CBrokerDataProvider* m_bound = nullptr;
};

} // namespace IBComm
