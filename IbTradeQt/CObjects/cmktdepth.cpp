#include "cmktdepth.h"

using namespace IBDataTypes;

CMktDepth::CMktDepth()
    : d(new CMktDepthData)
{

}

CMktDepth::CMktDepth(TickerId _id, qint32 _position, qint32 _operation, qint32 _side, qreal _price, qint32 _size, qint64 _timestamp)
    : d(new CMktDepthData)
{
    setId(_id);
    setPosition(_position);
    setOperation(_operation);
    setSide(_side);
    setPrice(_price);
    setSize(_size);
    setTimestamp(_timestamp);
}

CMktDepth & CMktDepth::operator=(const CMktDepth &other)
{
    d = other.d;
    return *this;
}


