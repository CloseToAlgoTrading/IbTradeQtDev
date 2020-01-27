#ifndef CTICKPRICE_H
#define CTICKPRICE_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CTickPriceData : public QSharedData
    {
    public:
	    CTickPriceData() : id(0)
		    , tickType(NOT_SET)
		    , price(0)
		    , canAutoExecute(0)
		    , timestamp(0)
	    {}

        CTickPriceData(const CTickPriceData &other) : QSharedData()
            , id(other.id)
		    , tickType(other.tickType)
		    , price(other.price)
		    , canAutoExecute(other.canAutoExecute)
		    , timestamp(other.timestamp)
	    {}

	    ~CTickPriceData(){}

	    /*
	    Parameter		Type		Description
	    id				TickerId	The ticker ID that was specified previously in the call to reqMktData()
	    tickType		TickeType
	    Specifies the type of price. Possible values are:

	    1 = bid
	    2 = ask
	    4 = last
	    6 = high
	    7 = low
	    9 = close
	    price			double		The bid, ask or last price, the daily high, daily low or last day close, depending on tickType value.
	    canAutoExecute	int			Specifies whether the price tick is available for automatic execution. Possible values are:

	    0 = not eligible for automatic execution
	    1 = eligible for automatic execution
	    */
	    TickerId   id;
	    TickType   tickType;
	    qreal      price;
	    qint32	   canAutoExecute;
	    qint64     timestamp;

    };

    //------------------------------------------------------
    class CMyTickPrice 
    {

    public:

        CMyTickPrice(const CMyTickPrice& other){
            d = other.d;
        }
        CMyTickPrice();
        ~CMyTickPrice() { }

        CMyTickPrice(TickerId _id, TickType _tickType, qreal _price, qint32 _canAutoExecute, qint64 _timestamp = 0/*, QObject *parent = 0*/);


        CMyTickPrice & operator= (const CMyTickPrice &other);

    private:
        QSharedDataPointer<CTickPriceData> d;

    public:
        TickerId getId() const { return d->id; }
        void setId(TickerId val) { d->id = val; }

        TickType getTickType() const { return d->tickType; }
        void setTickType(TickType val) { d->tickType = val; }

        qreal getPrice() const { return d->price; }
        void setPrice(qreal val) { d->price = val; }

        qint32 getCanAutoExecute() const { return d->canAutoExecute; }
        void setCanAutoExecute(qint32 val) { d->canAutoExecute = val; }

        qint64 getTimestamp() const { return d->timestamp; }
        void setTimestamp(qint64 val) { d->timestamp = val; }
        //int a;
    };
}
Q_DECLARE_METATYPE(IBDataTypes::CMyTickPrice);


#endif // CTICKPRICE_H
