#ifndef CMKTDEPTH_H
#define CMKTDEPTH_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CMktDepthData : public QSharedData
    {
    public:
        CMktDepthData() : Id(0)
            , position(0)
            , operation(0)
            , side(0)
            , price(0)
            , size(0)
            , timestamp(0)
        {}

        CMktDepthData(const CMktDepthData &other) : QSharedData(other)
            , Id(other.Id)
            , position(other.position)
            , operation(other.operation)
            , side(other.side)
            , price(other.price)
            , size(other.size)
            , timestamp(other.timestamp)

        {}

        ~CMktDepthData(){}

        //id		TickerId	The ticker ID that was specified previously in the call to reqMktDepth()
        //position	int			Specifies the row id of this market depth entry.
        //operation	int			Identifies the how this order should be applied to the market depth.Valid values are :
        //						0 = insert(insert this new order into the row identified by 'position')·
        //						1 = update(update the existing order in the row identified by 'position')·
        //						2 = delete (delete the existing order at the row identified by 'position')

        //side		int			Identifies the side of the book that this order belongs to.Valid values are :
        //						0 = ask
        //						1 = bid
        //price		double		The order price.
        //size		int			The order size.


        TickerId Id;
        qint32   position;
        qint32	 operation;
        qint32   side;
        qreal	 price;
        qint32   size;

        qint64      timestamp;

    };

    //------------------------------------------------------
    class CMktDepth
    {
    public:

        CMktDepth();
        CMktDepth(TickerId _id, qint32 _position, qint32 _operation, qint32 _side, qreal _price, qint32 _size, qint64 timestamp = 0);
        ~CMktDepth(){};

        CMktDepth(const CMktDepth& other){
            d = other.d;
        }

        CMktDepth & operator= (const CMktDepth &other);

    private:
        QSharedDataPointer<CMktDepthData> d;

    public:

        QSharedDataPointer<CMktDepthData> getMktDeptDataSharedPointer() const { return d; }
        void setMktDeptDataSharedPointer(QSharedDataPointer<CMktDepthData> val) { d = val; }

        TickerId getId() const { return d->Id; }
        void setId(TickerId val) { d->Id = val; }

        qint32 getPosition() const { return d->position; }
        void setPosition(qint32 val) { d->position = val; }

        qint32 getOperation() const { return d->operation; }
        void setOperation(qint32 val) { d->operation = val; }

        qint32 getSide() const { return d->side; }
        void setSide(qint32 val) { d->side = val; }

        qreal getPrice() const { return d->price; }
        void setPrice(qreal val) { d->price = val; }

        qint32 getSize() const { return d->size; }
        void setSize(qint32 val) { d->size = val; }

        qint64 getTimestamp() const { return d->timestamp; }
        void setTimestamp(qint64 val) { d->timestamp = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::CMktDepth);

#endif // CMKTDEPTH_H
