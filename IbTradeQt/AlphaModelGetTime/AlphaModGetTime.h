#ifndef ALPHAMODGETTIME_H
#define ALPHAMODGETTIME_H

#include <QObject>
#include <memory>
#include <QTimer>
//#include ".\IBComm\IBrokerAPI.h"
#include "Dispatcher.h"
#include "cbrokerdataprovider.h"
#include <QTime>


class AlphaModGetTime : public QObject, public Observer::CSubscriber
{
	Q_OBJECT

public:
    AlphaModGetTime(QObject *parent, CBrokerDataProvider & _refClient);
	~AlphaModGetTime();

	//void SeIBClient(IBrokerAPI& _mClient);


	void MessageHandler(void* pContext, tEReqType _reqType);
	void UnsubscribeHandler();

	void StartGetTimeUpdate(int period);
	void StopTimeUpdate();


	//IBrokerAPI& mClient;

    CBrokerDataProvider & m_Client;

public slots :
	void callbackTimer();

signals:
	void signalTimeReceived(long time);
    void signalPlanResetSubscribtion();


private:
	QTimer* timer;
    QTime resetTime;

};

#endif // ALPHAMODGETTIME_H
