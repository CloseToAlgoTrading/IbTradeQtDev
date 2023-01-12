#include "ctickprice.h"

using namespace IBDataTypes;

CMyTickPrice::CMyTickPrice()
    : d(new CTickPriceData)
{
    
}

CMyTickPrice::CMyTickPrice(TickerId _id, TickType _tickType, qreal _price, qint32 _canAutoExecute, qint64 _timestamp)
    : d(new CTickPriceData)
{
    setId(_id);
    setTickType(_tickType);
    setPrice(_price);
    setCanAutoExecute(_canAutoExecute);
    setTimestamp(_timestamp);

}

CMyTickPrice & CMyTickPrice::operator=(const CMyTickPrice &other)
{
    d = other.d;
    return *this;
}
