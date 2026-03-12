#ifndef ADAPTERS_ORDEREVENTBRIDGE_H
#define ADAPTERS_ORDEREVENTBRIDGE_H

#include <QObject>
#include <QString>
#include "IBOrderExecutionAdapter.h"

namespace Adapters {

class OrderEventBridge : public QObject {
    Q_OBJECT

public:
    explicit OrderEventBridge(IBOrderExecutionAdapter* adapter, QObject* parent = nullptr)
        : QObject(parent)
        , m_adapter(adapter) {}

public slots:
    void onOrderStatus(int orderId, const QString& status,
                       double filled, double remaining, double avgFillPrice)
    {
        Q_UNUSED(filled)
        Q_UNUSED(remaining)
        Q_UNUSED(avgFillPrice)
        if (m_adapter) {
            m_adapter->updateOrderStatus(orderId, status);
        }
    }

    void onExecDetails(int orderId, const QString& symbol,
                       double avgPrice, double shares)
    {
        Q_UNUSED(symbol)
        Q_UNUSED(avgPrice)
        Q_UNUSED(shares)
        if (m_adapter) {
            m_adapter->updateOrderStatus(orderId, "Filled");
        }
    }

private:
    IBOrderExecutionAdapter* m_adapter;
};

} // namespace Adapters

#endif // ADAPTERS_ORDEREVENTBRIDGE_H
