#include "crealtimebar.h"

using namespace IBDataTypes;

CrealtimeBar::CrealtimeBar()
    : d(new CrealtimeBarData)
{

}

CrealtimeBar & CrealtimeBar::operator=(const CrealtimeBar &other)
{
    d = other.d;
    return *this;
}


CrealtimeBar::CrealtimeBar(TickerId reqId, quint64 date, double open, double high, double low, double close, int volume, int Count, double WAP)
    : d(new CrealtimeBarData)
{
    setId(reqId);
    setDateTime(date);
    setOpen(open);
    setClose(close);
    setHigh(high);
    setLow(low);
    setVolume(volume);
    setWap(WAP);
    setCount(Count);

}
