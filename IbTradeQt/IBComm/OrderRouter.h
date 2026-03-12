#ifndef IBCOMM_ORDERROUTER_H
#define IBCOMM_ORDERROUTER_H

#include <QObject>
#include <QString>
#include <QMetaType>

namespace IBComm {

struct OrderStatusUpdate {
    Q_GADGET
    Q_PROPERTY(int orderId MEMBER orderId)
    Q_PROPERTY(QString status MEMBER status)
    Q_PROPERTY(double filled MEMBER filled)
    Q_PROPERTY(double remaining MEMBER remaining)
    Q_PROPERTY(double avgFillPrice MEMBER avgFillPrice)
public:
    int orderId = 0;
    QString status;
    double filled = 0.0;
    double remaining = 0.0;
    double avgFillPrice = 0.0;
};

struct ExecutionReport {
    Q_GADGET
    Q_PROPERTY(int orderId MEMBER orderId)
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double avgPrice MEMBER avgPrice)
    Q_PROPERTY(double shares MEMBER shares)
    Q_PROPERTY(QString execId MEMBER execId)
public:
    int orderId = 0;
    QString symbol;
    double avgPrice = 0.0;
    double shares = 0.0;
    QString execId;
};

struct CommissionUpdate {
    Q_GADGET
    Q_PROPERTY(QString execId MEMBER execId)
    Q_PROPERTY(double commission MEMBER commission)
    Q_PROPERTY(QString currency MEMBER currency)
    Q_PROPERTY(double realizedPnL MEMBER realizedPnL)
public:
    QString execId;
    double commission = 0.0;
    QString currency;
    double realizedPnL = 0.0;
};

class OrderRouter : public QObject {
    Q_OBJECT
public:
    explicit OrderRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onOrderStatus(int orderId, const QString& status, double filled,
                       double remaining, double avgFillPrice) {
        OrderStatusUpdate update;
        update.orderId = orderId;
        update.status = status;
        update.filled = filled;
        update.remaining = remaining;
        update.avgFillPrice = avgFillPrice;
        emit orderStatusChanged(update);
    }

    void onExecDetails(int orderId, const QString& symbol, double avgPrice,
                       double shares, const QString& execId) {
        ExecutionReport report;
        report.orderId = orderId;
        report.symbol = symbol;
        report.avgPrice = avgPrice;
        report.shares = shares;
        report.execId = execId;
        emit executionReceived(report);
    }

    void onCommissionReport(const QString& execId, double commission,
                            const QString& currency, double realizedPnL) {
        CommissionUpdate update;
        update.execId = execId;
        update.commission = commission;
        update.currency = currency;
        update.realizedPnL = realizedPnL;
        emit commissionReceived(update);
    }

    void onNextValidId(int orderId) {
        emit nextValidIdReceived(orderId);
    }

signals:
    void orderStatusChanged(const IBComm::OrderStatusUpdate& update);
    void executionReceived(const IBComm::ExecutionReport& report);
    void commissionReceived(const IBComm::CommissionUpdate& update);
    void nextValidIdReceived(int orderId);
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::OrderStatusUpdate)
Q_DECLARE_METATYPE(IBComm::ExecutionReport)
Q_DECLARE_METATYPE(IBComm::CommissionUpdate)

#endif // IBCOMM_ORDERROUTER_H
