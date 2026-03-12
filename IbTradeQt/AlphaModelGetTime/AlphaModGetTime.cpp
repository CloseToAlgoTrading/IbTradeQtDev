#include "AlphaModGetTime.h"
#include <QDebug>
#include "GlobalDef.h"


AlphaModGetTime::AlphaModGetTime(QObject *parent, CBrokerDataProvider & _refClient)
    : QObject(parent)
    , m_Client(_refClient)
    , timer(new QTimer(this))
    , resetTime(QTime(10, 00, 00))
{
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(callbackTimer()));
	timer->stop();
}

AlphaModGetTime::~AlphaModGetTime()
{
}

void AlphaModGetTime::slotCurrentTimeReceived(long time)
{
    emit signalTimeReceived(time);

    QTime current = QTime(QDateTime().fromMSecsSinceEpoch(static_cast<quint32>(time)).time());
    if(resetTime == current)
    {
        emit signalPlanResetSubscribtion();
    }
}

void AlphaModGetTime::StartGetTimeUpdate(int period)
{
    timer->start(period);
}

void AlphaModGetTime::StopTimeUpdate()
{
    timer->stop();
}

void AlphaModGetTime::callbackTimer()
{
    if(m_Client.isConnectedToTheServer())
    {
        m_Client.getClien()->reqCurrentTimeAPI();
    }
}
