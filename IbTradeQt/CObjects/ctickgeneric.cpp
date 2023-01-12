#include "ctickgeneric.h"

using namespace IBDataTypes;

CTickGeneric::CTickGeneric()
    : d(new CTickGenericData)
{

}

CTickGeneric::CTickGeneric(TickerId _id, TickType _tickType, qreal _value, qint64 _timestamp)
    : d(new CTickGenericData)
{
    setId(_id);
    setTickType(_tickType);
    setValue(_value);
    setTimestamp(_timestamp);
}

CTickGeneric & CTickGeneric::operator=(const CTickGeneric &other)
{
    d = other.d;
    return *this;
}
