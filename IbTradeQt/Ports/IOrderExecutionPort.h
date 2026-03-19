#ifndef PORTS_IORDEREXECUTIONPORT_H
#define PORTS_IORDEREXECUTIONPORT_H

#include <QString>
#include <QDateTime>
#include "../Common/Expected.h"
#include "../Pipeline/Contracts.h"

namespace Ports {

struct OrderResult {
    int orderId = 0;
    QString symbol;
    double quantity = 0.0;
    QString status;
    QDateTime timestamp;
};

class IOrderExecutionPort {
public:
    virtual ~IOrderExecutionPort() = default;

    virtual Expected<OrderResult, Error> placeOrder(
        const Pipeline::ExecutionIntent& intent) = 0;

    virtual Expected<void, Error> cancelOrder(int orderId) = 0;

    virtual Expected<OrderResult, Error> getOrderStatus(int orderId) = 0;

    virtual Expected<void, Error> cancelAllPending() { return {}; }
};

} // namespace Ports

#endif // PORTS_IORDEREXECUTIONPORT_H
