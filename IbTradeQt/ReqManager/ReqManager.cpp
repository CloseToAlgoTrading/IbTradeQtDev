#include "ReqManager.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcReqManager, "reqManager.general")

//----------------------------------------------------------------------------
ReqManager::ReqManager()
	: m_listIDs()
	, mapReqToResponseIds()
{

    QList<tEReqType> rqRelData = { RT_TICK_SIZE, RT_TICK_PRICE, RT_TICK_GENERIC, RT_TICK_STRING/*, RT_REALTIME_BAR*/
                                 , RT_REQ_OPTION_PRICE };
    mapReqToResponseIds.insert(RT_REQ_REL_DATA, rqRelData);

    QList<tEReqType> rqOrderData = { RT_ORDER_STATUS, RT_ORDER_EXECUTION, RT_ORDER_COMMISSION };
    mapReqToResponseIds.insert(RT_REQ_ORDER_STATUS, rqOrderData);

}

//----------------------------------------------------------------------------
ReqManager::~ReqManager()
{
}

//----------------------------------------------------------------------------
void ReqManager::addReqIds(qint32 reqId, tEReqType reqDataId)
{
	stReqIds tempReqId = { reqId, reqDataId };
    // check if Id and request data id exist in the list
	if (-1 == m_listIDs.indexOf(tempReqId))
	{
        // if not -> add to the list
		m_listIDs.push_back(tempReqId);
        qCDebug(lcReqManager) << "reqId =" << tempReqId.id << "reqType =" << tempReqId.reqType << "added to list!";
	}

}
//----------------------------------------------------------------------------
void ReqManager::removeReqIds(qint32 reqId, tEReqType reqDataId)
{
	stReqIds tempReqId = { reqId, reqDataId };

	m_listIDs.removeOne(tempReqId);
}

//----------------------------------------------------------------------------
bool ReqManager::isPresent(qint32 reqId, tEReqType reqDataId)
{
	bool ret = false;
	int foundIndex = 0;
	//TODO: добавить разбор ид -> если reqDataId = ticksize | tickdata ... => reqDataId = Req_RealTimeData..

	//mapReqToResonseIds

	stReqIds tempReqId = { reqId, reqDataId };


	QMap<tEReqType, QList<tEReqType>>::iterator it = mapReqToResponseIds.begin();
	for (; it != mapReqToResponseIds.end(); ++it)
	{
		foundIndex = it.value().indexOf(reqDataId);
		if (foundIndex != -1)
		{
			tempReqId.reqType = it.key();
		}
	}


	

	if (-1 != m_listIDs.indexOf(tempReqId))
	{
		ret = true;
		//qDebug("subscrId = %d, reqId = %d present in list!", tempReqId.id, tempReqId.reqId);
	}
	else
	{
		//qDebug("subscrId = %d, reqId = %d NOT in list!!", tempReqId.id, tempReqId.reqId);
	}

	return ret;
}

//----------------------------------------------------------------------------
qint32 ReqManager::getActiveRequestSize()
{
	return m_listIDs.size();
}

//----------------------------------------------------------------------------
void ReqManager::debugPrintList()
{
	QListIterator<stReqIds> Iter(m_listIDs);

	Iter.toFront();

	qCDebug(lcReqManager) << "Start List ----------------";

	while (Iter.hasNext())
	{
		stReqIds test = Iter.next();
		qCDebug(lcReqManager) << "subscrId =" << test.id << "reqId =" << test.reqType;
	}
	qCDebug(lcReqManager) << "---------------- End List";

}

QList<stReqIds> & ReqManager::getlistIDs()
{
	return m_listIDs;
}
