#ifndef CTICKSIZE_H
#define CTICKSIZE_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"


namespace IBDataTypes
{
    //------------------------------------------------------
    class CTickSizeData : public QSharedData
    {
    public:
        CTickSizeData() : id(0)
            , tickType(NOT_SET)
            , size(0)
            , timestamp(0)
        {}

        CTickSizeData(const CTickSizeData &other) : QSharedData(other)
            , id(other.id)
            , tickType(other.tickType)
            , size(other.size)
            , timestamp(other.timestamp)
        {}

        ~CTickSizeData(){}

        /*
        Parameter		Type		Description
        id				TickerId	The ticker ID that was specified previously in the call to reqMktData()
        tickType		TickType
        Specifies the type of size. Possible values are:
        0 = bid size
        3 = ask size
        5 = last size
        8 = volume
        size			int			Could be the bid size, ask size, last size or trading volume, depending on the tickType value.
        */

        TickerId    id;
        TickType	tickType;
        quint32		size;

        qint64      timestamp;

    };

    //------------------------------------------------------
    class CTickSize
    {

    public:
        CTickSize();
        CTickSize(TickerId _id, TickType _tickType, quint32 _size, qint64 _timestamp = 0);

        ~CTickSize(){};

        CTickSize(const CTickSize& other){
            d = other.d;
        }

        CTickSize & operator= (const CTickSize &other);
    private:
        QSharedDataPointer<CTickSizeData> d;

    public:

        TickerId getId() const { return d->id; }
        void setId(TickerId val) { d->id = val; }

        TickType getTickType() const { return d->tickType; }
        void setTickType(TickType val) { d->tickType = val; }

        quint32 getSize() const { return d->size; }
        void setSize(quint32 val) { d->size = val; }

        qint64 getTimestamp() const { return d->timestamp; }
        void setTimestamp(qint64 val) { d->timestamp = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::CTickSize);

#endif // CTICKSIZE_H
