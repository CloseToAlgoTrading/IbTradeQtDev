#ifndef CHISTORICALTICK_H
#define CHISTORICALTICK_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"
#include "HistoricalTick.h"

//namespace IBDataTypes
//{
//    //------------------------------------------------------
//    class CHistoricalTicksData : public QSharedData
//    {
//    public:
//        CHistoricalTicksData() : reqId(0)
//            //, HistoricalTick()
//            , done(false)
//        {}

//        CHistoricalTicksData(const CHistoricalTicksData &other) : QSharedData()
//            , reqId(other.reqId)
//            //, HistoricalTick(other.HistoricalTick)
//            , done(other.done)
//        {}

//        ~CHistoricalTicksData(){}

//        qint32 	reqId;
//        //HistoricalTick[] ticks;
//        bool 	done;

//    };

//    //------------------------------------------------------
//    class CHistoricalTicks
//    {

//    public:

//        CHistoricalTicks(const CHistoricalTicks& other){
//            d = other.d;
//        }
//        CHistoricalTicks();
//        ~CHistoricalTicks() { }

//        CHistoricalTicks(TickerId _id, TickType _tickType, qreal _price, qint32 _canAutoExecute, qint64 _timestamp = 0/*, QObject *parent = 0*/);


//        CHistoricalTicks & operator= (const CHistoricalTicks &other);

//    private:
//        QSharedDataPointer<CHistoricalTicksData> d;

//    public:
//        TickerId getId() const { return d->id; }
//        void setId(TickerId val) { d->id = val; }

//        TickType getTickType() const { return d->tickType; }
//        void setTickType(TickType val) { d->tickType = val; }

//        qreal getPrice() const { return d->price; }
//        void setPrice(qreal val) { d->price = val; }

//        qint32 getCanAutoExecute() const { return d->canAutoExecute; }
//        void setCanAutoExecute(qint32 val) { d->canAutoExecute = val; }

//        qint64 getTimestamp() const { return d->timestamp; }
//        void setTimestamp(qint64 val) { d->timestamp = val; }
//        //int a;
//    };
//}
//Q_DECLARE_METATYPE(IBDataTypes::CHistoricalTicks);


#endif // CHISTORICALTICK_H
