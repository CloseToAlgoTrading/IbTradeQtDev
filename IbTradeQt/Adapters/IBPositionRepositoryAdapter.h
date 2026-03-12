#ifndef ADAPTERS_IBPOSITIONREPOSITORYADAPTER_H
#define ADAPTERS_IBPOSITIONREPOSITORYADAPTER_H

#include "Ports/IPositionRepositoryPort.h"
#include "IBComm/PositionRouter.h"
#include <QObject>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>

class IBPositionRepositoryAdapter : public QObject, public Ports::IPositionRepositoryPort {
    Q_OBJECT
public:
    explicit IBPositionRepositoryAdapter(QObject* parent = nullptr)
        : QObject(parent) {}

    void connectToRouter(IBComm::PositionRouter* router) {
        connect(router, &IBComm::PositionRouter::positionChanged,
                this, &IBPositionRepositoryAdapter::onPositionChanged,
                Qt::QueuedConnection);
    }

    Expected<Ports::PositionRow, Error> getPosition(
        int strategyId, const QString& symbol) override
    {
        Q_UNUSED(strategyId)
        QMutexLocker lock(&m_mutex);
        auto it = m_positions.find(symbol);
        if (it == m_positions.end()) {
            return make_unexpected(Error{
                ErrorCode::NotFound, "Position not found",
                "IBPositionRepositoryAdapter::getPosition"
            });
        }
        return it.value();
    }

    Expected<QVector<Ports::PositionRow>, Error> getAllPositions(
        int strategyId) override
    {
        Q_UNUSED(strategyId)
        QMutexLocker lock(&m_mutex);
        QVector<Ports::PositionRow> result;
        for (const auto& row : m_positions)
            result.push_back(row);
        return result;
    }

    Expected<void, Error> updatePosition(
        const Ports::PositionRow& position) override
    {
        QMutexLocker lock(&m_mutex);
        m_positions[position.symbol] = position;
        return {};
    }

private slots:
    void onPositionChanged(const IBComm::PositionUpdate& update) {
        Ports::PositionRow row;
        row.symbol = update.symbol;
        row.quantity = update.quantity;
        row.avgCost = update.avgCost;
        row.account = update.account;
        QMutexLocker lock(&m_mutex);
        m_positions[update.symbol] = row;
    }

private:
    QMap<QString, Ports::PositionRow> m_positions;
    QMutex m_mutex;
};

#endif // ADAPTERS_IBPOSITIONREPOSITORYADAPTER_H
