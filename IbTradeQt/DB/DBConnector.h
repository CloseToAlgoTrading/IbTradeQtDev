#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include "baseimpl.h"
#include <QtSql>
#include "crealtimebar.h"
#include "ctickbytickalllast.h"

using namespace IBDataTypes;

class DBConnector : public QObject
{
	Q_OBJECT

public:
	DBConnector(QObject *parent);
	~DBConnector();
	
	bool connectDB();
	void disconnectDB();


    bool insertNewRealTimeBarItem(const CrealtimeBar* _item, const QString& _symbol);
//	bool updateItems();

public slots:
    void slotInsertNewRealTimeBarItem(const CrealtimeBar& _item, const QString& _symbol);
    void slotInsertTickByTickLastItem(const CTickByTickAllLast& _item, const QString& _symbol);



private:
    QSqlDatabase m_db;
	
};

#endif // DBCONNECTOR_H
