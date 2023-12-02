#ifndef CREALTIMEBAR_H
#define CREALTIMEBAR_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CrealtimeBarData : public QSharedData
    {
    public:
        CrealtimeBarData() : m_ReqId(0)
            , m_DateTime(0)
            , m_Open(0)
            , m_Higt(0)
            , m_Low(0)
            , m_Close(0)
            , m_Volume(0)
            , m_Wap(0)
            , m_Count(0)
        {}

        CrealtimeBarData(const CrealtimeBarData &other) : QSharedData()
            , m_ReqId(other.m_ReqId)
            , m_DateTime(other.m_DateTime)
            , m_Open(other.m_Open)
            , m_Higt(other.m_Higt)
            , m_Low(other.m_Low)
            , m_Close(other.m_Close)
            , m_Volume(other.m_Volume)
            , m_Wap(other.m_Wap)
            , m_Count(other.m_Count)
        {}

        ~CrealtimeBarData(){}


        //	reqId	the requests identifier
        //	date	the bar's date and time (either as a yyyymmss hh:mm:ss formatted string or as system time according to the request)
        //	open	the bar's open point
        //	high	the bar's high point
        //	low	the bar's low point
        //	close	the bar's closing point
        //	volume	the bar's traded volume if available
        //	WAP	the bar's Weighted Average Price
        //	count	the number of trades during the bar's timespan (only available for TRADES).

        TickerId m_ReqId;
        quint64	m_DateTime;
        qreal	m_Open;
        qreal	m_Higt;
        qreal	m_Low;
        qreal	m_Close;
        qreal   m_Volume;
        qreal   m_Wap;
        qint32  m_Count;

    };

    //------------------------------------------------------
    class CrealtimeBar
    {

    public:
        CrealtimeBar();
        CrealtimeBar(TickerId reqId, quint64 date, double open, double high, double low, double close, int volume, int Count, double WAP/*, QObject *parent = NULL*/);
        ~CrealtimeBar(){}

        CrealtimeBar(const CrealtimeBar& other){
            d = other.d;
        }

        CrealtimeBar & operator= (const CrealtimeBar &other) ;

        bool operator == (const CrealtimeBar &c1) const {
            return d == c1.d;
        }

    private:
        QSharedDataPointer<CrealtimeBarData> d;

    public:
        QSharedDataPointer<CrealtimeBarData> getRTBarDataSharedPointer() const { return d; }
        void setRTBarDataSharedPointer(QSharedDataPointer<CrealtimeBarData> val) { d = val; }

        TickerId getId() const { return d->m_ReqId; }
        void setId(TickerId val) { d->m_ReqId = val; }

        quint64 getDateTime() const { return d->m_DateTime; }
        void setDateTime(quint64 val) { d->m_DateTime = val; }

        qreal getOpen() const { return d->m_Open; }
        void setOpen(double val) { d->m_Open = val; }

        qreal getClose() const { return d->m_Close; }
        void setClose(double val) { d->m_Close = val; }

        qreal getHigh() const { return d->m_Higt; }
        void setHigh(double val) { d->m_Higt = val; }

        qreal getLow() const { return d->m_Low; }
        void setLow(double val) { d->m_Low = val; }

        qreal getVolume() const { return d->m_Volume; }
        void setVolume(double val) { d->m_Volume = val; }

        qreal getWap() const { return d->m_Wap; }
        void setWap(double val) { d->m_Wap = val; }

        qint32 getCount() const { return d->m_Count; }
        void setCount(qint32 val) { d->m_Count = val; }
    };
}

Q_DECLARE_METATYPE(IBDataTypes::CrealtimeBar);
#endif // CREALTIMEBAR_H
