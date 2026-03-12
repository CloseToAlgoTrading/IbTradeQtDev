#ifndef ADAPTERS_MOCKEXECUTIONADAPTER_H
#define ADAPTERS_MOCKEXECUTIONADAPTER_H

#include "Ports/IOrderExecutionPort.h"
#include <QVector>

class MockExecutionAdapter : public Ports::IOrderExecutionPort {
public:
    Expected<Ports::OrderResult, Error> placeOrder(
        const Pipeline::ExecutionIntent& intent) override
    {
        if (m_shouldFail) {
            return make_unexpected(Error{
                m_failCode, m_failMessage, "MockExecutionAdapter::placeOrder"
            });
        }

        Ports::OrderResult result;
        result.orderId = m_nextOrderId++;
        result.symbol = intent.symbol;
        result.quantity = intent.quantity;
        result.status = "Filled";
        result.timestamp = intent.timestamp.isValid()
            ? intent.timestamp : QDateTime::currentDateTime();

        m_placedOrders.push_back(result);
        return result;
    }

    Expected<void, Error> cancelOrder(int orderId) override {
        for (int i = 0; i < m_placedOrders.size(); ++i) {
            if (m_placedOrders[i].orderId == orderId) {
                m_cancelledOrderIds.push_back(orderId);
                m_placedOrders[i].status = "Cancelled";
                return {};
            }
        }
        return make_unexpected(Error{
            ErrorCode::NotFound, "Order not found", "MockExecutionAdapter::cancelOrder"
        });
    }

    Expected<Ports::OrderResult, Error> getOrderStatus(int orderId) override {
        for (const auto& order : m_placedOrders) {
            if (order.orderId == orderId) {
                return order;
            }
        }
        return make_unexpected(Error{
            ErrorCode::NotFound, "Order not found", "MockExecutionAdapter::getOrderStatus"
        });
    }

    // Test inspection
    const QVector<Ports::OrderResult>& placedOrders() const { return m_placedOrders; }
    const QVector<int>& cancelledOrderIds() const { return m_cancelledOrderIds; }
    int orderCount() const { return m_placedOrders.size(); }

    // Test control
    void setFailOnPlace(ErrorCode code, const std::string& message) {
        m_shouldFail = true;
        m_failCode = code;
        m_failMessage = message;
    }
    void clearFail() { m_shouldFail = false; }

    void reset() {
        m_placedOrders.clear();
        m_cancelledOrderIds.clear();
        m_nextOrderId = 1;
        m_shouldFail = false;
    }

private:
    int m_nextOrderId = 1;
    QVector<Ports::OrderResult> m_placedOrders;
    QVector<int> m_cancelledOrderIds;
    bool m_shouldFail = false;
    ErrorCode m_failCode = ErrorCode::UnknownError;
    std::string m_failMessage;
};

#endif // ADAPTERS_MOCKEXECUTIONADAPTER_H
