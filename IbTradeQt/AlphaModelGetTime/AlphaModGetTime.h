#ifndef ALPHAMODGETTIME_H
#define ALPHAMODGETTIME_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include "cbrokerdataprovider.h"

class AlphaModGetTime : public QObject
{
	Q_OBJECT

public:
    AlphaModGetTime(QObject *parent, CBrokerDataProvider & _refClient);
	~AlphaModGetTime();

	void StartGetTimeUpdate(int period);
	void StopTimeUpdate();

    CBrokerDataProvider & m_Client;

public slots:
	void callbackTimer();
    void slotCurrentTimeReceived(long time);

signals:
	void signalTimeReceived(long time);
    void signalPlanResetSubscribtion();
    void signalServerStateChanged(bool isConnected);

private:
	QTimer* timer;
    QTime resetTime;
};

#endif // ALPHAMODGETTIME_H
