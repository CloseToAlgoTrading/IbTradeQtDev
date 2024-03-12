#ifndef CORDERSTATUS_H
#define CORDERSTATUS_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{

    //------------------------------------------------------
    class COrderStatusData : public QSharedData
    {
    public:
        COrderStatusData() : m_id(0)
            , m_status("")
            , m_filled(0)
            , m_remaining(0)
            , m_avgFillPrice(0)
            , m_permId(0)
            , m_parentId(0)
            , m_lastFilledPrice(0)
            , m_clientId(0)
            , m_whyHeld("")
            , m_ExecId("")
            , m_Direction(OA_BUY)
        {}

        COrderStatusData(const COrderStatusData &other) : QSharedData(other)
            , m_id(other.m_id)
            , m_status(other.m_status)
            , m_filled(other.m_filled)
            , m_remaining(other.m_remaining)
            , m_avgFillPrice(other.m_avgFillPrice)
            , m_permId(other.m_permId)
            , m_parentId(other.m_parentId)
            , m_lastFilledPrice(other.m_lastFilledPrice)
            , m_clientId(other.m_clientId)
            , m_whyHeld(other.m_whyHeld)
            , m_ExecId(other.m_ExecId)
            , m_Direction(other.m_Direction)
        {}

        ~COrderStatusData(){}

        // id		OrderId		The order ID that was specified previously in the call to placeOrder()

        // status	IBString	The order status.Possible values include :
        //						PendingSubmit - indicates that you have transmitted the order, but have not  yet received confirmation that it has been accepted by the order destination.NOTE : This order status is not sent by TWS and should be explicitly set by the API developer when an order is submitted.
        //						PendingCancel - indicates that you have sent a request to cancel the order but have not yet received cancel confirmation from the order destination.At this point, your order is not confirmed canceled.You may still receive an execution while your cancellation request is pending.NOTE : This order status is not sent by TWS and should be explicitly set by the API developer when an order is canceled.
        //						PreSubmitted - indicates that a simulated order type has been accepted by the IB system and that this order has yet to be elected.The order is held in the IB system until the election criteria are met.At that time the order is transmitted to the order destination as specified.
        //						Submitted - indicates that your order has been accepted at the order destination and is working.
        //						Cancelled - indicates that the balance of your order has been confirmed canceled by the IB system.This could occur unexpectedly when IB or the destination has rejected your order.
        //						Filled - indicates that the order has been completely filled.
        //						Inactive - indicates that the order has been accepted by the system(simulated orders) or an exchange(native orders) but that currently the order is inactive due to system, exchange or other issues.

        // filled	int			Specifies the number of shares that have been executed.
        //						For more information about partial fills, see Order Status for Partial Fills.

        // remaining		int		Specifies the number of shares still outstanding.
        // avgFillPrice		double	The average price of the shares that have been executed.This parameter is valid only if the filled parameter value is greater than zero.Otherwise, the price parameter will be zero.
        // permId			int		The TWS id used to identify orders.Remains the same over TWS sessions.
        // parentId			int		The order ID of the parent order, used for bracket and auto trailing stop orders.
        // lastFilledPrice	double	The last price of the shares that have been executed.This parameter is valid only if the filled parameter value is greater than zero.Otherwise, the price parameter will be zero.
        // clientId			int		The ID of the client(or TWS) that placed the order.Note that TWS orders have a fixed clientId and orderId of 0 that distinguishes them from API orders.
        // whyHeld			IBString	This field is used to identify an order held when TWS is trying to locate shares for a short sell.The value used to indicate this is 'locate'.

        qint32	m_id;
        QString m_status;
        qreal	m_filled;
        qreal	m_remaining;
        qreal	m_avgFillPrice;
        qint32	m_permId;
        qint32	m_parentId;
        qreal	m_lastFilledPrice;
        qint32	m_clientId;
        QString m_whyHeld;

        QString m_ExecId;
        eOrderAction_t m_Direction;

    };
    //------------------------------------------------------
    class COrderStatus
    {
    public:
        COrderStatus();
        COrderStatus(qint32	_id,
            QString _status,
            qreal	_filled,
            qreal	_remaining,
            qreal	_avgFillPrice,
            qint32	_permId,
            qint32	_parentId,
            qreal	_lastFilledPrice,
            qint32	_clientId,
            QString _whyHeld,
            QString _ExecId,
            eOrderAction_t _Direction
            );
        ~COrderStatus(){}

        COrderStatus(const COrderStatus& other){
            d = other.d;
        }

        COrderStatus & operator= (const COrderStatus &other);

    private:
        QSharedDataPointer<COrderStatusData> d;

    public:
        qint32 getId() const { return d->m_id; }
        void setId(qint32 val) { d->m_id = val; }
        QString getStatus() const { return d->m_status; }
        void setStatus(QString val) { d->m_status = val; }
        qreal getFilled() const { return d->m_filled; }
        void setFilled(qreal val) { d->m_filled += val; d->m_remaining -= val;}
        qreal getRemaining() const { return d->m_remaining; }
        void setRemaining(qreal val) { d->m_remaining = val; }
        qreal getAvgFillPrice() const { return d->m_avgFillPrice; }
        void setAvgFillPrice(qreal val) { d->m_avgFillPrice = val; }
        qint32 getPermId() const { return d->m_permId; }
        void setPermId(qint32 val) { d->m_permId = val; }
        qint32 getParentId() const { return d->m_parentId; }
        void setParentId(qint32 val) { d->m_parentId = val; }
        qreal getLastFilledPrice() const { return d->m_lastFilledPrice; }
        void setLastFilledPrice(qreal val) { d->m_lastFilledPrice = val; }
        qint32 getClientId() const { return d->m_clientId; }
        void setClientId(qint32 val) { d->m_clientId = val; }
        QString getWhyHeld() const { return d->m_whyHeld; }
        void setWhyHeld(QString val) { d->m_whyHeld = val; }
        QString getExecId() const { return d->m_ExecId; }
        void setExecId(QString val) { d->m_ExecId = val; }
        eOrderAction_t getDirection() const { return d->m_Direction; }
        void setDirection(eOrderAction_t val) { d->m_Direction = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::COrderStatus);

#endif // CORDERSTATUS_H
