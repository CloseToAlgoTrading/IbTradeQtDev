#ifndef PORTS_IPOSITIONREPOSITORYPORT_H
#define PORTS_IPOSITIONREPOSITORYPORT_H

#include <QString>
#include <QVector>
#include "../Common/Expected.h"

namespace Ports {

struct PositionRow {
    QString symbol;
    double quantity = 0.0;
    double avgCost = 0.0;
    QString account;
    int strategyId = 0;
};

class IPositionRepositoryPort {
public:
    virtual ~IPositionRepositoryPort() = default;

    virtual Expected<PositionRow, Error> getPosition(
        int strategyId,
        const QString& symbol) = 0;

    virtual Expected<QVector<PositionRow>, Error> getAllPositions(
        int strategyId) = 0;

    virtual Expected<void, Error> updatePosition(
        const PositionRow& position) = 0;
};

} // namespace Ports

#endif // PORTS_IPOSITIONREPOSITORYPORT_H
