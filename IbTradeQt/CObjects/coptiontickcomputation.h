#ifndef COPTIONTICKCOMPUTATION_H
#define COPTIONTICKCOMPUTATION_H

#include <QObject>
#include <QSharedData>


#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class COptionTickComputationData : public QSharedData
    {
    public:
        COptionTickComputationData() : tickerId(0)
          , field(0)
          , impliedVolatility(0)
          , delta(0)
          , optPrice(0)
          , pvDividend(0)
          , gamma(0)
          , vega(0)
          , theta(0)
          , undPrice(0)
        {}

        COptionTickComputationData(const COptionTickComputationData &other) : tickerId(other.tickerId)
          , field(other.field)
          , impliedVolatility(other.impliedVolatility)
          , delta(other.delta)
          , optPrice(other.optPrice)
          , pvDividend(other.pvDividend)
          , gamma(other.gamma)
          , vega(other.vega)
          , theta(other.theta)
          , undPrice(other.undPrice)
        {}

        ~COptionTickComputationData(){}

        qint32 	tickerId;
        qint32 	field;
        qreal   impliedVolatility;
        qreal 	delta;
        qreal 	optPrice;
        qreal 	pvDividend;
        qreal 	gamma;
        qreal 	vega;
        qreal 	theta;
        qreal 	undPrice;


    };
    //------------------------------------------------------
    class COptionTickComputation
    {
    public:
        COptionTickComputation(void);
        COptionTickComputation(qint32 	tickerId,
                               qint32 	field,
                               qreal   impliedVolatility,
                               qreal 	delta,
                               qreal 	optPrice,
                               qreal 	pvDividend,
                               qreal 	gamma,
                               qreal 	vega,
                               qreal 	theta,
                               qreal 	undPrice
            );
        ~COptionTickComputation(){}

        COptionTickComputation(const COptionTickComputation& other){
            d = other.d;
        }

        COptionTickComputation & operator= (const COptionTickComputation &other);

    private:
        QSharedDataPointer<COptionTickComputationData> d;

    public:
        qint32 	get_tickerId() const {return d->tickerId ;}
        qint32 	get_field() const {return d->field ;}
        qreal   get_impliedVolatility() const {return d->impliedVolatility ;}
        qreal 	get_delta() const {return d->delta ;}
        qreal 	get_optPrice() const {return d->optPrice ;}
        qreal 	get_pvDividend() const {return d->pvDividend ;}
        qreal 	get_gamma() const {return d->gamma ;}
        qreal 	get_vega() const {return d->vega ;}
        qreal 	get_theta() const {return d->theta ;}
        qreal 	get_undPrice() const {return d->undPrice ;}

        void 	set_tickerId(qint32 val) {d->tickerId = val;}
        void 	set_field(qint32 val) {d->field = val;}
        void   set_impliedVolatility(qreal val) {d->impliedVolatility = val;}
        void 	set_delta(qreal val) {d->delta = val;}
        void 	set_optPrice(qreal val) {d->optPrice = val;}
        void 	set_pvDividend(qreal val) {d->pvDividend = val;}
        void 	set_gamma(qreal val) {d->gamma = val;}
        void 	set_vega(qreal val) {d->vega = val;}
        void 	set_theta(qreal val) {d->theta = val;}
        void 	set_undPrice(qreal val) {d->undPrice = val;}

    };
}

Q_DECLARE_METATYPE(IBDataTypes::COptionTickComputation);


#endif // COPTIONTICKCOMPUTATION_H
