#include "cticksize.h"


using namespace IBDataTypes;

CTickSize::CTickSize()
    : d(new CTickSizeData)
{

}

CTickSize::CTickSize(TickerId _id, TickType _tickType, quint32 _size, qint64 _timestamp )
    : d(new CTickSizeData)
{
    setId(_id);
    setTickType(_tickType);
    setSize(_size);
    setTimestamp(_timestamp);
}

CTickSize & CTickSize::operator=(const CTickSize &other)
{
    d = other.d;
    return *this;
}

