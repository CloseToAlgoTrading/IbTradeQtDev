#ifndef GLOBALREQMANAGER_H
#define GLOBALREQMANAGER_H

#include <./ReqManager/ReqManager.h>


//typedef QMultiMap<QString, stReqIds>		TikerIdMap_t;
typedef QMap<QString, stReqIds>		TikerIdMap_t;

//TODO: check inputs parameters

/***************************************************************
* \brief	Implements full Request Manager functionality
*           Class used in SubscriberItem class, each subscriber
*           has own Request Manager in the observer
****************************************************************/
class GlobalReqManager : public ReqManager
{
public:
    GlobalReqManager();
    ~GlobalReqManager();

    /***************************************************************
    * \brief	Function return next free ID,
    *           that can be used for request
    * \returns	Not used request ID
    ****************************************************************/
    qint32 getNextFreeId();

    /***************************************************************
    * \brief        Function return next free ID
    * \param[in]	QString     Requested Symbol Name
    * \param[in]	tEReqType	Request Data Type Id
    * \returns      Already used Id for requested symbol and requested
    *               data type or not used ID
    ****************************************************************/
    qint32 getNextFreeId(const QString &_symbol, tEReqType _reqType);

    /***************************************************************
    * \brief        Function checks if requested id is free
    * \param[in]	qint32      Request Id
    * \returns      True - id free, False - id is not free
    ****************************************************************/
    bool isIdFree(qint32 _reqId);

    /***************************************************************
    * \brief        Function adds symbol and request data to the
    *               active request Ticker map and to active request list
    * \param[in]	QString     Request symbol
    * \param[in]	stReqIds	Request Data
    * \returns      None
    ****************************************************************/
    void addReqIdsExt(QString _symbol, stReqIds _reqData);

    /***************************************************************
    * \brief        Function removes symbol and request data from the
    *               active Ticker map and from the active request list
    * \param[in]	QString     Request symbol
    * \param[in]	stReqIds	Request Data
    * \returns      True - Ok (always), False - Some error
    ****************************************************************/
    bool removeReqIdsExt(QString _symbol, stReqIds _reqData);

    /***************************************************************
    * \brief        Function returns request Id by the Symbol and
    *               request data type
    * \param[in]	QString     Request symbol
    * \param[in]	tEReqType	Request Data Type
    * \returns      request Id, or 0 if not found
    ****************************************************************/
    qint32 getIdbySymbol(QString _symbol, tEReqType _reqType);

    /***************************************************************
    * \brief        Function returns request Symbol name by the Id and
    *               request data type
    * \param[in]	qint32      Request Id
    * \param[in]	tEReqType	Request Data Type
    * \returns      Symbol name or empty string
    ****************************************************************/
    QString getSymbolById(const qint32 _reqId, const tEReqType _reqType);

    /***************************************************************
    * \brief        Function removes request with Symbol name and
    *               request data type
    * \param[in]	QString     Symbol name
    * \param[in]	tEReqType	Request Data Type
    * \returns      Id was removed or 0
    ****************************************************************/
    qint32 removeReqData(const QString & _symbol, const tEReqType _t);

    /***************************************************************
    * \brief        Function returns request data by Symbol name and
    *               request data type
    * \param[in]	QString     Symbol name
    * \param[in]	tEReqType	Request Data Type
    * \param[out]	stReqIds	return active requested data
    * \returns      True - data valid, False - data not valid
    ****************************************************************/
    bool getReqData(QString _symbol, tEReqType _reqType, stReqIds & _ret);

    /***************************************************************
    * \brief        Function removes request with Symbol name and
    *               request data from the ticker map
    * \param[in]	QString     Symbol name
    * \param[in]	stReqIds	Request Data
    * \returns      count of the rest items, 0 if there are no items in ticker map
    ****************************************************************/
    quint32 removeItemFromTickerMap(const QString _symbol, const stReqIds & _reqData);

    /***************************************************************
    * \brief        Function return count of items in ticker map
    * \param[in]	QString     Symbol name
    * \param[in]	stReqIds	Request Data
    * \returns      count of the items in the ticker map
    ****************************************************************/
    quint32 getItemsCountInTickerMap(const QString _symbol, const stReqIds & _reqData);

private:

    //map contatins map symbols name to id:reqDataTypeId struct
    TikerIdMap_t m_tickerMap;
	
};

#endif // GLOBALREQMANAGER_H
