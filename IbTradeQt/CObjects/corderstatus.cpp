#include "corderstatus.h"
using namespace IBDataTypes;


COrderStatus::COrderStatus()
    : d(new COrderStatusData)
{

}

COrderStatus::COrderStatus(qint32 _id, QString _status, qint32 _filled, qreal _remaining, qreal _avgFillPrice, qint32 _permId, qint32 _parentId, qreal _lastFilledPrice, qint32 _clientId, QString _whyHeld )
    : d(new COrderStatusData)
{
    setId(_id);
    setFilled(_filled);
    setStatus(_status);
    setRemaining(_remaining);
    setAvgFillPrice(_avgFillPrice);
    setPermId(_permId);
    setParentId(_parentId);
    setLastFilledPrice(_lastFilledPrice);
    setClientId(_clientId);
    setWhyHeld("");
}

COrderStatus & COrderStatus::operator=(const COrderStatus &other)
{
    d = other.d;
    return *this;
}


