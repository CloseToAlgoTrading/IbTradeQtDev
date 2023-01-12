#include "cposition.h"
using namespace IBDataTypes;


CPosition::CPosition(void) : d(new CPositionData)
{

}

CPosition::CPosition(QString  _account,
                     Contract _contract,
                     qreal    _pos,
                     qreal    _avgCost) : d(new CPositionData)
{
    setAccount(_account);
    setContract(_contract);
    setPos(_pos);
    setAvgCost(_avgCost);
}

CPosition & CPosition::operator=(const CPosition &other)
{
    d = other.d;
    return *this;
}


