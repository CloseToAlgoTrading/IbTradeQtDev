#ifndef IBCOMM_MARKETDEPTHROUTER_H
#define IBCOMM_MARKETDEPTHROUTER_H

#include <QObject>
#include <QMap>
#include <QMetaType>

namespace IBComm {

struct DepthUpdate {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(int position MEMBER position)
    Q_PROPERTY(int operation MEMBER operation)
    Q_PROPERTY(int side MEMBER side)
    Q_PROPERTY(double price MEMBER price)
    Q_PROPERTY(double size MEMBER size)
public:
    QString symbol;
    int position = 0;
    int operation = 0;
    int side = 0;
    double price = 0.0;
    double size = 0.0;
};

struct DepthL2Update {
    Q_GADGET
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(int position MEMBER position)
    Q_PROPERTY(QString marketMaker MEMBER marketMaker)
    Q_PROPERTY(int operation MEMBER operation)
    Q_PROPERTY(int side MEMBER side)
    Q_PROPERTY(double price MEMBER price)
    Q_PROPERTY(double size MEMBER size)
    Q_PROPERTY(bool isSmartDepth MEMBER isSmartDepth)
public:
    QString symbol;
    int position = 0;
    QString marketMaker;
    int operation = 0;
    int side = 0;
    double price = 0.0;
    double size = 0.0;
    bool isSmartDepth = false;
};

class MarketDepthRouter : public QObject
{
    Q_OBJECT
public:
    explicit MarketDepthRouter(QObject* parent = nullptr) : QObject(parent) {}

    void registerReqIdSymbol(int reqId, const QString& symbol) {
        m_reqIdToSymbol[reqId] = symbol;
    }

    void onDepthUpdate(int reqId, int position, int operation, int side,
                       double price, double size) {
        DepthUpdate d;
        d.symbol = m_reqIdToSymbol.value(reqId);
        d.position = position;
        d.operation = operation;
        d.side = side;
        d.price = price;
        d.size = size;
        emit depthUpdated(d);
    }

    void onDepthL2Update(int reqId, int position, const QString& marketMaker,
                         int operation, int side, double price, double size,
                         bool isSmartDepth) {
        DepthL2Update d;
        d.symbol = m_reqIdToSymbol.value(reqId);
        d.position = position;
        d.marketMaker = marketMaker;
        d.operation = operation;
        d.side = side;
        d.price = price;
        d.size = size;
        d.isSmartDepth = isSmartDepth;
        emit depthL2Updated(d);
    }

signals:
    void depthUpdated(const IBComm::DepthUpdate& update);
    void depthL2Updated(const IBComm::DepthL2Update& update);

private:
    QMap<int, QString> m_reqIdToSymbol;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::DepthUpdate)
Q_DECLARE_METATYPE(IBComm::DepthL2Update)

#endif // IBCOMM_MARKETDEPTHROUTER_H
