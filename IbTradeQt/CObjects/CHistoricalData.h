#ifndef CHISTORICALDATA
#define CHISTORICALDATA

#include <QObject>
#include <QList>

#include <QDateTime>
#include "GlobalDef.h"
#include "crealtimebar.h"
namespace IBDataTypes
{

    //------------------------------------------------------
    class CHistoricalDataSharedData : public QSharedData //public CrealtimeBarData
    {
    public:
        CHistoricalDataSharedData() //: CrealtimeBarData()
            : m_hasGaps(0)
            , m_isLast(false)
        {}

        CHistoricalDataSharedData(const CHistoricalDataSharedData &other) //: CrealtimeBarData(static_cast<CrealtimeBarData>(other))
            : QSharedData()
            , m_hasGaps(other.m_hasGaps)
            , m_isLast(other.m_isLast)
        {}

        ~CHistoricalDataSharedData(){}

        qint32  m_hasGaps;
        bool m_isLast;
    };

    //------------------------------------------------------
    class CHistoricalData : public CrealtimeBar
    {

    public:
        CHistoricalData();
        CHistoricalData(TickerId reqId, QString date, double open, double high, double low, double close, int volume, int barCount, double WAP, int hasGaps, bool isLast = false);
        ~CHistoricalData(){}

        CHistoricalData(const CHistoricalData& other) : CrealtimeBar(other)
        {
            this->setRTBarDataSharedPointer(other.getRTBarDataSharedPointer());
            d = other.d;
        }

        CHistoricalData & operator= (const CHistoricalData &other);

    private:
        QSharedDataPointer<CHistoricalDataSharedData> d;

    public:

        qint32 getHasGaps() const { return d->m_hasGaps; }
        void setHasGaps(qint32 val) { d->m_hasGaps = val; }

        bool getIsLast() const { return d->m_isLast; }
        void setIsLast(bool val) { d->m_isLast = val; }

    };

}

Q_DECLARE_METATYPE(IBDataTypes::CHistoricalData);

#endif // CHISTORICALDATA
