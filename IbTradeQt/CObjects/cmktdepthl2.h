#ifndef CMKTDEPTHL2_H
#define CMKTDEPTHL2_H

#include <QObject>
#include "cmktdepth.h"
#include "GlobalDef.h"
#include <QString>

namespace IBDataTypes
{
    //------------------------------------------------------
    class CMktDepthL2Data : public QSharedData
    {
    public:
        CMktDepthL2Data() : marketMaker("")
        {}

        CMktDepthL2Data(const CMktDepthL2Data &other) : QSharedData(other)
            , marketMaker(other.marketMaker)
        {}

        ~CMktDepthL2Data(){}

        //marketMaker	IBString	Specifies the exchange hosting this order
        QString marketMaker;

    };

    //------------------------------------------------------
    class CMktDepthL2 : public CMktDepth
    {

    public:
        CMktDepthL2();
        CMktDepthL2(TickerId _id, qint32 _position, QString _marketMaker, qint32 _operation, qint32 _side, qreal _price, qint32 _size, qint64 _timestamp = 0);
        ~CMktDepthL2(){};

        CMktDepthL2(const CMktDepthL2& other):CMktDepth(other){
            this->setMktDeptDataSharedPointer(other.getMktDeptDataSharedPointer());
            d = other.d;
        }

        CMktDepthL2 & operator= (const CMktDepthL2 &other);

    private:
        QSharedDataPointer<CMktDepthL2Data> d;

    public:
        QString getMarketMaker() const { return d->marketMaker; }
        void setMarketMaker(QString val) { d->marketMaker = val; }
    };
}

Q_DECLARE_METATYPE(IBDataTypes::CMktDepthL2);

#endif // CMKTDEPTHL2_H
