#include "CHistoricalData.h"
#include "NHelper.h"
using namespace IBDataTypes;
using namespace NHelper;

CHistoricalData::CHistoricalData()
	: CrealtimeBar()
    , d(new CHistoricalDataSharedData)
{

}

CHistoricalData::CHistoricalData(TickerId reqId, QString date, double open, double high, double low, double close, int volume, int barCount, double WAP, int hasGaps, bool isLast)
	: CrealtimeBar(reqId, NHelper::convertTimeStringToTimestamp(date, true), open, high, low, close, volume, barCount, WAP)
    , d(new CHistoricalDataSharedData)
{
    setHasGaps(hasGaps);
    setIsLast(isLast);
}

CHistoricalData & IBDataTypes::CHistoricalData::operator=(const CHistoricalData &other)
{
    this->setRTBarDataSharedPointer(other.getRTBarDataSharedPointer());
    d = other.d;
    return *this;
}

