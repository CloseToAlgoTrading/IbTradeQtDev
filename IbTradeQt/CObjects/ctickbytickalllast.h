#ifndef CTICKBYTICKALLLAST_H
#define CTICKBYTICKALLLAST_H


#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CTickByTickAllLastData : public QSharedData
    {
    public:
        CTickByTickAllLastData() : reqId(0)
          , tickType(0)
          , time(0)
          , price(0)
          , size(0)
          , tickAttriblast()
          , exchange()
          , specialConditions()
        {}

        CTickByTickAllLastData(const CTickByTickAllLastData &other) : QSharedData(other)
            , reqId(other.reqId)
            , tickType(other.tickType)
            , time(other.time)
            , price(other.price)
            , size(other.size)
            , tickAttriblast(other.tickAttriblast)
            , exchange(other.exchange)
            , specialConditions(other.specialConditions)

        {}

        ~CTickByTickAllLastData(){}

        qint64 	reqId;
        qint32 	tickType;
        qint64 	time;
        qreal 	price;
        qint32 	size;
        TickAttribLast 	tickAttriblast;
        QString 	exchange;
        QString 	specialConditions;

    };

    //------------------------------------------------------
    class CTickByTickAllLast
    {
    public:

        CTickByTickAllLast();
        CTickByTickAllLast(qint64 reqId, qint32 tickType, qint64 time, qreal price, qint32 size,
                           TickAttribLast tickAttriblast, QString exchange, QString	specialConditions);
        ~CTickByTickAllLast(){}

        CTickByTickAllLast(const CTickByTickAllLast& other){
            d = other.d;
        }

        CTickByTickAllLast & operator= (const CTickByTickAllLast &other);

    private:
        QSharedDataPointer<CTickByTickAllLastData> d;

    public:

        QSharedDataPointer<CTickByTickAllLastData> getCTickByTickAllLastDataSharedPointer() const { return d; }
        void setCTickByTickAllLastDataSharedPointer(QSharedDataPointer<CTickByTickAllLastData> val) { d = val; }

        auto getId() const { return d->reqId; }
        void setId(qint64 val) { d->reqId = val; }

        auto getExchange() const { return d->exchange; }
        void setExchange(QString val) { d->exchange = val; }

        auto getPrice() const { return d->price; }
        void setPrice(qreal val) { d->price = val; }

        auto getSize() const { return d->size; }
        void setSize(qint32 val) { d->size = val; }

        auto getSpecialConditions() const { return d->specialConditions; }
        void setSpecialConditions(QString val) { d->specialConditions = val; }

        auto getTickAttribLast() const { return d->tickAttriblast; }
        void setTickAttribLast(TickAttribLast val) { d->tickAttriblast = val; }

        auto getTimestamp() const { return d->time; }
        void setTimestamp(qint64 val) { d->time = val; }

        auto getTickType() const { return d->tickType; }
        void setTtickType(qint32 val) { d->tickType = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::CTickByTickAllLast);


#endif // CTICKBYTICKALLLAST_H
