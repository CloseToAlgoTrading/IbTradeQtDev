#include "cmktdepthl2.h"

using namespace IBDataTypes;

CMktDepthL2::CMktDepthL2()
    : CMktDepth()
    , d(new CMktDepthL2Data)
{

}

CMktDepthL2::CMktDepthL2(TickerId _id, qint32 _position, QString _marketMaker, qint32 _operation, qint32 _side, qreal _price, qint32 _size, qint64 _timestamp)
	: CMktDepth(_id, _position, _operation, _side, _price, _size, _timestamp)
    , d(new CMktDepthL2Data)
{
    setMarketMaker(_marketMaker);
}


CMktDepthL2 & CMktDepthL2::operator=(const CMktDepthL2 &other)
{
    this->setMktDeptDataSharedPointer(other.getMktDeptDataSharedPointer());
    d = other.d;
    return *this;
}


