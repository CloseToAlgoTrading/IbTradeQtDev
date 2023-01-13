#ifndef CTICKGENERIC_H
#define CTICKGENERIC_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CTickGenericData : public QSharedData
    {
    public:
        CTickGenericData() : id(0)
            , tickType(NOT_SET)
            , value(0)
            , timestamp(0)
        {}

        CTickGenericData(const CTickGenericData &other) : QSharedData(other)
            , id(other.id)
            , tickType(other.tickType)
            , value(other.value)
            , timestamp(other.timestamp)
        {}

        ~CTickGenericData(){}

        //	Parameter	Type	Description
        //	tickerId	int	    The ticker Id that was specified previously in the call to reqMktData()
        //	tickType	int 	Specifies the type of price.
        //	                    Pass the field value into TickType.getField(int tickType) to retrieve the field description.For example, a field value of 46 will map to shortable, etc.
        //	value	    double	The value of the specified field

        TickerId   id;
        TickType   tickType;
        qreal      value;

        qint64      timestamp;

    };

    //------------------------------------------------------
    class CTickGeneric
    {

    public:
        CTickGeneric(const CTickGeneric& other){
            d = other.d;
        }

        CTickGeneric();
        CTickGeneric(TickerId _id, TickType _tickType, qreal _value, qint64 _timestamp = 0);
        ~CTickGeneric(){};

        CTickGeneric & operator= (const CTickGeneric &other);

    private:
        QSharedDataPointer<CTickGenericData> d;

    public:

        TickerId getId() const { return d->id; }
        void setId(TickerId val) { d->id = val; }

        TickType getTickType() const { return d->tickType; }
        void setTickType(TickType val) { d->tickType = val; }

        qreal getValue() const { return d->value; }
        void setValue(qreal val) { d->value = val; }

        qint64 getTimestamp() const { return d->timestamp; }
        void setTimestamp(qint64 val) { d->timestamp = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::CTickGeneric);

#endif // CTICKGENERIC_H
