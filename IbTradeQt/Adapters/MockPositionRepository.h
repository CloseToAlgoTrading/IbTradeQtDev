#ifndef ADAPTERS_MOCKPOSITIONREPOSITORY_H
#define ADAPTERS_MOCKPOSITIONREPOSITORY_H

#include "Ports/IPositionRepositoryPort.h"
#include <QMap>

class MockPositionRepository : public Ports::IPositionRepositoryPort {
public:
    Expected<Ports::PositionRow, Error> getPosition(
        int strategyId, const QString& symbol) override
    {
        QString key = makeKey(strategyId, symbol);
        auto it = m_positions.find(key);
        if (it == m_positions.end()) {
            return make_unexpected(Error{
                ErrorCode::NotFound, "Position not found", "MockPositionRepository"
            });
        }
        return it.value();
    }

    Expected<QVector<Ports::PositionRow>, Error> getAllPositions(int strategyId) override {
        QVector<Ports::PositionRow> result;
        for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
            if (it.value().strategyId == strategyId) {
                result.push_back(it.value());
            }
        }
        return result;
    }

    Expected<void, Error> updatePosition(const Ports::PositionRow& position) override {
        QString key = makeKey(position.strategyId, position.symbol);
        m_positions[key] = position;
        return {};
    }

    // Test helpers
    void setPosition(int strategyId, const QString& symbol,
                     double quantity, double avgCost = 0.0)
    {
        Ports::PositionRow row;
        row.strategyId = strategyId;
        row.symbol = symbol;
        row.quantity = quantity;
        row.avgCost = avgCost;
        m_positions[makeKey(strategyId, symbol)] = row;
    }

    void reset() { m_positions.clear(); }
    int positionCount() const { return m_positions.size(); }

private:
    static QString makeKey(int strategyId, const QString& symbol) {
        return QString("%1:%2").arg(strategyId).arg(symbol);
    }

    QMap<QString, Ports::PositionRow> m_positions;
};

#endif // ADAPTERS_MOCKPOSITIONREPOSITORY_H
