#include "ctickbytickalllast.h"


using namespace IBDataTypes;

CTickByTickAllLast::CTickByTickAllLast()
    : d(new CTickByTickAllLastData)
{

}

CTickByTickAllLast::CTickByTickAllLast(qint64 reqId,
                                                    qint32 tickType,
                                                    qint64 time,
                                                    qreal price,
                                                    qint32 size,
                                                    TickAttribLast tickAttriblast,
                                                    QString exchange,
                                                    QString specialConditions)
    : d(new CTickByTickAllLastData)
{
    setId(reqId);
    setTtickType(tickType);
    setTimestamp(time);
    setPrice(price);
    setSize(size);
    setTickAttribLast(tickAttriblast);
    setExchange(exchange);
    setSpecialConditions(specialConditions);
}


CTickByTickAllLast &IBDataTypes::CTickByTickAllLast::operator=(const IBDataTypes::CTickByTickAllLast &other)
{
    d = other.d;
    return *this;
}
