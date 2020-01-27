#include "copenorder.h"

using namespace IBDataTypes;

COpenOrder::COpenOrder()
    : d(new COpenOrderData)
{

}

COpenOrder & COpenOrder::operator=(const COpenOrder &other)
{
    d = other.d;
    return *this;
}

