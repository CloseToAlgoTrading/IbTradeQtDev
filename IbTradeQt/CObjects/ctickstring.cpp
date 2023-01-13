#include "ctickstring.h"

using namespace IBDataTypes;

CTickString::CTickString()
    :d(new CTickStringData)
{

}

CTickString::CTickString(TickerId _id, TickType _tickType, QString _value, qint64 _timestamp )
    : d(new CTickStringData)
{
    setId(_id);
    setTickType(_tickType);
    setValue(_value);
    setTimestamp(_timestamp);
}

CTickString & CTickString::operator=(const CTickString &other)
{
    d = other.d;
    return *this;
}


