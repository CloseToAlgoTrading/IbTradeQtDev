#include "globalreqmanager.h"

GlobalReqManager::GlobalReqManager()
	: ReqManager()
	, m_tickerMap()
{
}

//--------------------------------------------------------------------------------
GlobalReqManager::~GlobalReqManager()
{

}

//--------------------------------------------------------------------------------
qint32 GlobalReqManager::getNextFreeId()
{
	qint32 retId = 0;
	qint32 maxId (std::numeric_limits<qint32>::max() - 1);
	bool isFound = false;
    /* checks numbers until found not used id */
	for (qint32 i = 1; ((i < maxId) && (!isFound)); i++)
	{
		if (isIdFree(i))
		{
			isFound = true;
			retId = i;
		}
	}
	
	return retId;
}


//--------------------------------------------------------------------------------
qint32 GlobalReqManager::getNextFreeId(const QString &_symbol, tEReqType _reqType)
{
	qint32 id = 0;
    /* checks if symbol and id already requested */
	id = getIdbySymbol(_symbol, _reqType);
	if (0 == id)
	{
        /* if not -> get next free id from the pool*/
		id = getNextFreeId();
	}

	return id;
}


//--------------------------------------------------------------------------------
bool GlobalReqManager::isIdFree(qint32 _reqId)
{
	bool ret = true;
	
    //Todo: replace QList by QVector for better performance
	QList<stReqIds>::iterator it = m_listIDs.begin();
    //Check if Id is not used yet
	for (; it != m_listIDs.end(); ++it)
	{
		if (it->id == _reqId)
		{
            //set to false if id exist in the list
            //todo: exit from the cycle
			ret = false;
		}
	}

	return ret;

}

//-------------------------------------------------------------------------
void GlobalReqManager::addReqIdsExt(QString _symbol, stReqIds _reqData)
{
    /* Add data to the active ticker map */
	m_tickerMap.insert(_symbol, _reqData);
    /* Add data to active id list */
	addReqIds(_reqData.id, _reqData.reqType);
}

//-------------------------------------------------------------------------
bool GlobalReqManager::removeReqIdsExt(QString _symbol, stReqIds _reqData)
{
	bool ret = false;

    // remove item from the ticker map
    quint32 retActiveItems = removeItemFromTickerMap(_symbol, _reqData);

    if (0 == retActiveItems)
    {
        // if no more items with the same symbol-reqType -> remove from the active request list
        removeReqIds(_reqData.id, _reqData.reqType);
    }

	ret = true;

	return ret;
}

//-------------------------------------------------------------------------
qint32 GlobalReqManager::getIdbySymbol(QString _symbol, tEReqType _reqType)
{
	qint32 retId = 0;
	bool isFound = false;
	//if (RT_REQ_REL_DATA == _reqType)
	{
        // get all symbols in the ticker map
		TikerIdMap_t::iterator itMap = m_tickerMap.find(_symbol);
        // iterate over all founded items until we find the item with the requested type
		while (itMap != m_tickerMap.end() && itMap.key() == _symbol && !isFound) {
            if (itMap.value().reqType == _reqType/* RT_REQ_REL_DATA*/)
			{
				retId = itMap.value().id;
				isFound = true;
			}
			++itMap;
		}
	}
    // return found id or 0
	return retId;
}

//-------------------------------------------------------------------------
QString GlobalReqManager::getSymbolById(const qint32 _Id, const tEReqType _reqType)
{
	QString retSymbol = "";
	bool isFound = false;
	///if (RT_REQ_REL_DATA == _reqType)
	{
		TikerIdMap_t::iterator itMap = m_tickerMap.begin();
        // iterate over all ticker map until we get combitation of id and request data type
		while (itMap != m_tickerMap.end() && !isFound) {
			if ((itMap.value().reqType == _reqType) && (itMap.value().id ==  _Id))
			{
				retSymbol = itMap.key();
				isFound = true;
			}
			++itMap;
		}
	}

	return retSymbol;
}

//-------------------------------------------------------------------------
qint32 GlobalReqManager::removeReqData(const QString & _symbol, const tEReqType _t)
{
    qint32 ret = 0;

    // get all items
    TikerIdMap_t::iterator itMap = m_tickerMap.find(_symbol);
    while ((0 == ret) && (itMap != m_tickerMap.end()) && (itMap.key() == _symbol)) {
        if (itMap.value().reqType == _t)
        {
            ret = itMap->id;
            removeReqIdsExt(_symbol, { itMap->id, itMap->reqType });
        }
        else
        {
            ++itMap;
        }

    }

    return ret;
}

//-------------------------------------------------------------------------
bool GlobalReqManager::getReqData(QString _symbol, tEReqType _reqType, stReqIds & _ret)
{
	bool _res = false;

	TikerIdMap_t::iterator itMap = m_tickerMap.find(_symbol);
	while ((false == _res) && (itMap != m_tickerMap.end()) && (itMap.key() == _symbol)) {
        if (itMap.value().reqType == _reqType)
		{
			_res = true;
			_ret = itMap.value();
		}
		else
		{
			++itMap;
		}
	}

	return _res;

}

//-------------------------------------------------------------------------
quint32 GlobalReqManager::removeItemFromTickerMap(const QString _symbol, const stReqIds & _reqData)
{
    bool isFound = false;
    quint32 retFoundItems = 0;
    QString symbolToRemove = _symbol;
    TikerIdMap_t::iterator itemToRemove;

    // find symbol if it is empty
    if (_symbol.isEmpty())
    {
        symbolToRemove = getSymbolById(_reqData.id, _reqData.reqType);
    }


    //searching for the data in ticker map
    for (auto it = m_tickerMap.begin(); it != m_tickerMap.end(); ++it) {
        if ((it.value() == _reqData) && (it.key() == symbolToRemove))
        {
            if (!isFound)
            {
                //store first founded item
                isFound = true;
                itemToRemove = it;
            }
            else
            {
                //increment counter of founded items
                ++retFoundItems;
            }
        }
    }

    if (isFound)
    {
        // remove stored item
        m_tickerMap.erase(itemToRemove);
    }

    // return count of the rest items with the same characteristic in ticker map
    return retFoundItems;
}

//-------------------------------------------------------------------------
quint32 GlobalReqManager::getItemsCountInTickerMap(const QString _symbol, const stReqIds & _reqData)
{
    quint32 retFoundItems = 0;
    QString symbolToRemove = _symbol;

    if (_symbol.isEmpty())
    {
        symbolToRemove = getSymbolById(_reqData.id, _reqData.reqType);
    }

    for (auto it = m_tickerMap.begin(); it != m_tickerMap.end(); ++it) {
        if ((it.value() == _reqData) && (it.key() == symbolToRemove))
        {
            ++retFoundItems;
        }
    }

    return retFoundItems;
}
