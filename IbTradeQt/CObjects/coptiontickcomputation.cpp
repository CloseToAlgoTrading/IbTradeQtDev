#include "coptiontickcomputation.h"


using namespace IBDataTypes;


COptionTickComputation::COptionTickComputation(void) : d(new COptionTickComputationData)
{

}

COptionTickComputation::COptionTickComputation(qint32 tickerId,
                                                qint32 field,
                                                qreal impliedVolatility,
                                                qreal delta,
                                                qreal optPrice,
                                                qreal pvDividend,
                                                qreal gamma,
                                                qreal vega,
                                                qreal theta,
                                                qreal undPrice) : d(new COptionTickComputationData)
{
    set_tickerId(tickerId);
    set_field(field);
    set_impliedVolatility(impliedVolatility);
    set_delta(delta);
    set_optPrice(optPrice);
    set_pvDividend(pvDividend);
    set_gamma(gamma);
    set_vega(vega);
    set_theta(theta);
    set_undPrice(undPrice);
}

COptionTickComputation & COptionTickComputation::operator=(const COptionTickComputation &other)
{
    d = other.d;
    return *this;
}


