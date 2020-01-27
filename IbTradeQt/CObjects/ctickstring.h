#ifndef CTICKSTRING_H
#define CTICKSTRING_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"


namespace IBDataTypes
{
    //------------------------------------------------------
    class CTickStringData : public QSharedData
    {
    public:
        CTickStringData() : id(0)
            , tickType(NOT_SET)
            , value("")
            , timestamp(0)
        {}

        CTickStringData(const CTickStringData &other) : id(other.id)
            , tickType(other.tickType)
            , value(other.value)
            , timestamp(other.timestamp)
        {}

        ~CTickStringData(){}

        // Parameter	Type	    Description
        // tickerId	    TickerId	The ticker Id that was specified previously in the call to reqMktData().
        // field	    TickType    Specifies the type of price.
        //                          Pass the field value into TickType.getField(int tickType) to retrieve the field description.For example, a field value of 45 will map to lastTimestamp, etc.
        // value	    IBString	The value of the specified field.

        TickerId    id;
        TickType	tickType;
        QString		value;

        qint64      timestamp;

    };

    //------------------------------------------------------
    class CTickString : public QObject
    {

    public:
        CTickString();
        CTickString(TickerId _id, TickType _tickType, QString _value, qint64 _timestamp = 0);

        ~CTickString(){}

        CTickString(const CTickString& other){
            d = other.d;
        }

        CTickString & operator= (const CTickString &other);
    private:
        QSharedDataPointer<CTickStringData> d;

    public:

        TickerId getId() const { return d->id; }
        void setId(TickerId val) { d->id = val; }

        TickType getTickType() const { return d->tickType; }
        void setTickType(TickType val) { d->tickType = val; }

        QString getValue() const { return d->value; }
        void setValue(QString val) { d->value = val; }

        qint64 getTimestamp() const { return d->timestamp; }
        void setTimestamp(qint64 val) { d->timestamp = val; }

    };

}

Q_DECLARE_METATYPE(IBDataTypes::CTickString);

#endif // CTICKSTRING_H
