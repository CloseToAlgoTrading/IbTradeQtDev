#ifndef COPENORDER_H
#define COPENORDER_H

//TODO: REMOVE #include "StdAfx.h" // need for Order.h and OrderState.h
#include "Contract.h"
#include "Order.h"
#include "OrderState.h"


#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class COpenOrderData : public QSharedData
    {
    public:
        COpenOrderData() : m_orderID(0)
            , m_contract()
            , m_order()
            , m_orderState()
        {}

        COpenOrderData(const COpenOrderData &other) : QSharedData(other)
            , m_orderID(other.m_orderID)
            , m_contract(other.m_contract)
            , m_order(other.m_order)
            , m_orderState(other.m_orderState)

        {}

        ~COpenOrderData(){}

        //orderID		OrderId		The order ID assigned by TWS.Use to cancel or update the order.
        //contract		Contract	The Contract class attributes describe the contract.
        //order			Order		The Order class gives the details of the open order.
        //orderState	OrderState	The orderState class includes attributes used for both pre and post trade margin and commission data.

        qint32		m_orderID;
        Contract	m_contract;
        Order		m_order;
        OrderState	m_orderState;

    };

    class COpenOrder
    {
    public:
        COpenOrder();
        ~COpenOrder();

        COpenOrder(const COpenOrder& other){
            d = other.d;
        }

        COpenOrder & operator= (const COpenOrder &other);

    private:
        QSharedDataPointer<COpenOrderData> d;
    public:
        qint32 getOrderID() const { return d->m_orderID; }
        void setOrderID(qint32 val) { d->m_orderID = val; }
        const Contract & getContract() const { return d->m_contract; }
        void setContract(Contract val) { d->m_contract = val; }
        const Order & getOrder() const { return d->m_order; }
        void setOrder(Order val) { d->m_order = val; }
        const OrderState & getOrderState() const { return d->m_orderState; }
        void setOrderState(OrderState val) { d->m_orderState = val; }
    };
}

Q_DECLARE_METATYPE(IBDataTypes::COpenOrder);

#endif // COPENORDER_H
