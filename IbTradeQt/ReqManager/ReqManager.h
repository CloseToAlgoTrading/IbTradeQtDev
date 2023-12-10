#ifndef REQMANAGER_H
#define REQMANAGER_H

#include <QList>
#include <QMap>


#define N_MIN_VALUE -10

enum tENonPermReqId{
    E_RQ_ID_TIME = N_MIN_VALUE,
    E_RQ_ID_POSITION,
    E_RQ_ID_HISTORICAL,
    E_RQ_ID_RESTART_SUBSCRIPTION,
    E_RQ_ID_ORDER_STATUS,

    E_RQ_ID_ERROR_SUBSCRIPTION

};


/* Request Data Types */
enum tEReqType {
    RT_REQ_NONE = 0,
    RT_REQ_REL_DATA,
    RT_REQ_CUR_TIME,
    RT_TICK_SIZE,
    RT_TICK_PRICE,
    RT_TICK_GENERIC,
    RT_TICK_STRING,
    RT_HISTORICAL_DATA,
    RT_HISTORICAL_TICK_DATA,
    RT_TICK_BY_TICK_DATA,
    RT_REALTIME_BAR,
    RT_MKT_DEPTH,
    RT_MKT_DEPTH_L2,
    RT_REQ_POSITION,
    RT_POSITION,
    RT_REQ_OPTION_PRICE,
    RT_REQ_RESTART_SUBSCRIPTION,
    RT_REQ_ERROR_SUBSRIPTION,

    RT_REQ_ORDER_STATUS,
    RT_ORDER_STATUS,
    RT_ORDER_COMMISSION,
    RT_ORDER_EXECUTION,

    RT_NEXT_VALID_ID
};

#pragma once

struct stReqIds
{
	qint32 id;
	tEReqType reqType;

	bool operator==(const stReqIds &val) const
	{
		if ((id == val.id) && (reqType == val.reqType))
		{
			return true;
		}
		return false;
	}
};


/***************************************************************
* \brief	Basic functionality add/remove Ids
****************************************************************/
class ReqManager
{
public:
    ReqManager();
    ~ReqManager();

    /***************************************************************
    * \brief	Function adds item into the m_listIDs if is not there
    * \param[in]	qint32      Request Id
    * \param[in]	tEReqType	Request Data Type Id
    * \returns	None
    ****************************************************************/
    void addReqIds(qint32 reqId, tEReqType reqDataId);

    /***************************************************************
    * \brief	Function removes item from the m_listIDs
    * \param[in]	qint32      Request Id
    * \param[in]	tEReqType	Request Data Type Id
    * \returns	None
    ****************************************************************/
    void removeReqIds(qint32 reqId, tEReqType reqDataId);

    /***************************************************************
    * \brief	Function checks if item exists in the m_listIDs
    * \param[in]	qint32      Request Id
    * \param[in]	tEReqType	Request Data Type Id
    * \returns	None
    ****************************************************************/
    bool isPresent(qint32 reqId, tEReqType reqDataId);

    /***************************************************************
    * \brief	Function returns size of the m_listIDs
    * \returns	size of list
    ****************************************************************/
    qint32 getActiveRequestSize();

    /***************************************************************
    * \brief	Function prints all members of m_listIDs
    * \returns	None
    ****************************************************************/
    void debugPrintList();
    /***************************************************************
    * \brief	Function prints all members of m_listIDs
    * \returns	None
    ****************************************************************/
    QList<stReqIds> & getlistIDs();

protected:
    /* List contains request IDs and request data type ids. */
    QList<stReqIds> m_listIDs;

    /* Mapping between Request Data Type and another Response Data Types.
     * so that we can inform subscribers with one response to different
     * responses from the server */
    QMap<tEReqType, QList<tEReqType>> mapReqToResponseIds;
};

#endif
