#ifndef ADAPTERS_IBORDEREXECUTIONADAPTER_H
#define ADAPTERS_IBORDEREXECUTIONADAPTER_H

#include "Ports/IOrderExecutionPort.h"
#include "IBComm/IBrokerAPI.h"
#include <QMap>

class IBOrderExecutionAdapter : public Ports::IOrderExecutionPort {
public:
    explicit IBOrderExecutionAdapter(IBrokerAPI* brokerApi)
        : m_brokerApi(brokerApi) {}

    Expected<Ports::OrderResult, Error> placeOrder(
        const Pipeline::ExecutionIntent& intent) override
    {
        if (!m_brokerApi || !m_brokerApi->isConnectedAPI()) {
            return make_unexpected(Error{
                ErrorCode::BrokerConnectionFailed,
                "Not connected to broker",
                "IBOrderExecutionAdapter::placeOrder"
            });
        }

        eOrderAction_t action = (intent.quantity >= 0) ? OA_BUY : OA_SELL;
        qint32 qty = static_cast<qint32>(std::abs(intent.quantity));

        qint32 orderId = m_brokerApi->reqPlaceOrderAPI(
            intent.symbol, qty, action);

        if (orderId <= 0) {
            return make_unexpected(Error{
                ErrorCode::OrderRejected,
                "Broker returned invalid order ID",
                "IBOrderExecutionAdapter::placeOrder"
            });
        }

        Ports::OrderResult result;
        result.orderId = orderId;
        result.symbol = intent.symbol;
        result.quantity = intent.quantity;
        result.status = "Submitted";
        result.timestamp = QDateTime::currentDateTime();

        m_activeOrders[orderId] = result;
        return result;
    }

    Expected<void, Error> cancelOrder(int orderId) override {
        if (!m_brokerApi || !m_brokerApi->isConnectedAPI()) {
            return make_unexpected(Error{
                ErrorCode::BrokerConnectionFailed,
                "Not connected to broker",
                "IBOrderExecutionAdapter::cancelOrder"
            });
        }

        m_brokerApi->cancelOrderAPI(static_cast<qint32>(orderId));

        if (m_activeOrders.contains(orderId)) {
            m_activeOrders[orderId].status = "PendingCancel";
        }

        return {};
    }

    Expected<Ports::OrderResult, Error> getOrderStatus(int orderId) override {
        auto it = m_activeOrders.find(orderId);
        if (it == m_activeOrders.end()) {
            return make_unexpected(Error{
                ErrorCode::NotFound,
                "Order not tracked",
                "IBOrderExecutionAdapter::getOrderStatus"
            });
        }
        return it.value();
    }

    void updateOrderStatus(int orderId, const QString& status) {
        if (m_activeOrders.contains(orderId)) {
            m_activeOrders[orderId].status = status;
        }
    }

private:
    IBrokerAPI* m_brokerApi;
    QMap<int, Ports::OrderResult> m_activeOrders;
};

#endif // ADAPTERS_IBORDEREXECUTIONADAPTER_H
