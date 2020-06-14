#ifndef BASEIMPL_H
#define BASEIMPL_H

#include <QObject>
//#include ".\IBComm\IBrokerAPI.h"
#include "Dispatcher.h"

class BaseImpl : public QObject, public Observer::CSubscriber
{
	Q_OBJECT

public:
	BaseImpl(QObject *parent);
	~BaseImpl();

	virtual void UnsubscribeHandler();
	//virtual void SeIBClient(IBrokerAPI& _mClient);
	//IBrokerAPI& mClient;

private:
	
};


#endif // BASEIMPL_H

