#ifndef BASEIMPL_H
#define BASEIMPL_H

#include <QObject>

class BaseImpl : public QObject
{
	Q_OBJECT

public:
	BaseImpl(QObject *parent);
	~BaseImpl();

private:
	
};


#endif // BASEIMPL_H
