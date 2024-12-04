#include "AlphaModGetTime.h"
#include <QDebug>
#include "GlobalDef.h"


AlphaModGetTime::AlphaModGetTime(QObject *parent, CBrokerDataProvider & _refClient)
    : QObject(parent)
    , m_Client(_refClient)
    , timer(new QTimer(this))
    , resetTime(QTime(10, 00, 00))
{

	//Подключаем сигнал таймера к слоту, которым выступает наша функция
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(callbackTimer()));

	//устанавливаем таймер в 0
	timer->stop();
}

AlphaModGetTime::~AlphaModGetTime()
{

}

void AlphaModGetTime::UnsubscribeHandler()
{

}


void AlphaModGetTime::MessageHandler(void* pContext, tEReqType _reqType)
{

    if (RT_REQ_CUR_TIME == _reqType)
    {
        long _time = (*static_cast<long*>(pContext));
        emit signalTimeReceived(_time);

        QTime current = QTime(QDateTime().fromMSecsSinceEpoch(static_cast<quint32>(_time)).time());
        if(resetTime == current)
        {
            emit signalPlanResetSubscribtion();
        }
    }
};


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
        //qDebug() << "Update request current time timer...";
        //m_Client.Subscribe(this, E_RQ_ID_TIME, RT_REQ_CUR_TIME);
        stReqIds r = { E_RQ_ID_TIME, RT_REQ_CUR_TIME };
        m_Client.Subscribe(this, TimeSymbol, r);
        m_Client.getClien()->reqCurrentTimeAPI();
    }
}
