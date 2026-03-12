#ifndef IBCOMM_POSITIONROUTER_H
#define IBCOMM_POSITIONROUTER_H

#include <QObject>
#include <QMap>
#include <QMetaType>

namespace IBComm {

struct PositionUpdate {
    Q_GADGET
    Q_PROPERTY(QString account MEMBER account)
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double quantity MEMBER quantity)
    Q_PROPERTY(double avgCost MEMBER avgCost)
public:
    QString account;
    QString symbol;
    double quantity = 0.0;
    double avgCost = 0.0;
};

class PositionRouter : public QObject {
    Q_OBJECT
public:
    explicit PositionRouter(QObject* parent = nullptr) : QObject(parent) {}

    PositionUpdate lastPosition(const QString& symbol) const {
        return m_positions.value(symbol);
    }

    QMap<QString, PositionUpdate> allPositions() const { return m_positions; }

public slots:
    void onPosition(const QString& account, const QString& symbol,
                    double position, double avgCost) {
        PositionUpdate update;
        update.account = account;
        update.symbol = symbol;
        update.quantity = position;
        update.avgCost = avgCost;
        m_positions[symbol] = update;
        emit positionChanged(update);
    }

    void onPositionEnd() {
        emit positionSnapshotComplete();
    }

signals:
    void positionChanged(const IBComm::PositionUpdate& update);
    void positionSnapshotComplete();

private:
    QMap<QString, PositionUpdate> m_positions;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::PositionUpdate)

#endif // IBCOMM_POSITIONROUTER_H
