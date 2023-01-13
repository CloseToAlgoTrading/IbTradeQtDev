#include "Dispatcher.h"

namespace Observer
{
	//CDispatcher::CDispatcher()
	//{
	//}


	//CDispatcher::~CDispatcher()
	//{
	//}
    //--------------------------------------------------------
    const CDispatcher::CSubscriberItemPtr CDispatcher::getSubscriberItem(SubscriberId _subscriberId)
    {
        if (nullptr != m_pSubscriberList)
        {
            for (size_t i = 0; i < m_pSubscriberList->size(); ++i)
            {
                CSubscriberItemPtr pSubscriberItem = (*m_pSubscriberList)[i];
                if (_subscriberId == pSubscriberItem->Subscriber()->GetSubscriberId())
                {
                    return pSubscriberItem;
                }
            }
        }
        else
        {
            //ToDo: add log messages to dispatcher
        }


        return nullptr;
    }

    //---------------------------------------------------------
    //Observer::SubscriberId CDispatcher::Subscribe(CSubscriberPtr pNewSubscriber, const qint32 reqId, const tEReqType reqDataId)
//    {
//        QString s = "NULL";
//        stReqIds r = { reqId, reqDataId };

//        return Subscribe(pNewSubscriber, s, r);
//    }

    //---------------------------------------------------------
    Observer::SubscriberId CDispatcher::Subscribe(CSubscriberPtr pNewSubscriber, const QString & _symbol, const stReqIds & _reqData)
    {
        //Declaration of the next shared pointer before ScopeLocker
        //prevents release of subscribers from under lock
        CSubscriberListPtr pNewSubscriberList(new CSubscriberList());

        //Enter to locked section
        std::lock_guard<std::mutex> lock(g_i_mutex);

        if ((nullptr != m_pSubscriberList) && (!m_pSubscriberList->empty()))
        {
            //Copy existing subscribers
            pNewSubscriberList->assign(m_pSubscriberList->begin(), m_pSubscriberList->end());
        }


        for (size_t i = 0; i < pNewSubscriberList->size(); ++i)
        {
            CSubscriberItemPtr pSubscriberItem = (*pNewSubscriberList)[i];
            if (pSubscriberItem->Subscriber()->GetSubscriberId() == pNewSubscriber->GetSubscriberId())
            {
                pSubscriberItem->ReqMenager()->addReqIdsExt(_symbol, _reqData);
                return pSubscriberItem->Subscriber()->GetSubscriberId();
            }
        }

        //Add new subscriber to new subscriber list
        CSubscriberItemPtr pTempSbcrItem(new CSubscriberItem(pNewSubscriber));
        pTempSbcrItem->ReqMenager()->addReqIdsExt(_symbol, _reqData);
        pNewSubscriberList->push_back(pTempSbcrItem);



        //Exchange subscriber lists
        m_pSubscriberList = pNewSubscriberList;

        return pNewSubscriber->GetSubscriberId();
    }
    
    //---------------------------------------------------------
    bool CDispatcher::Unsubscribe(SubscriberId id, const qint32 reqId, const tEReqType reqDataId, const QString _s)
    {
        stReqIds r = { reqId, reqDataId };

        return Unsubscribe(id, _s, r);
    }

    //---------------------------------------------------------
    bool CDispatcher::Unsubscribe(SubscriberId id, const QString & _symbol, const stReqIds & _reqData)
    {
        //Declaration of the next shared pointers before ScopeLocker
        //prevents release of subscribers from under lock
        CSubscriberItemPtr  pSubscriberItemToRelease;
        CSubscriberListPtr  pNewSubscriberList;

        //Enter to locked section (scope)
        std::lock_guard<std::mutex> lock(g_i_mutex);

        if ((nullptr == m_pSubscriberList) || (m_pSubscriberList->empty()))
        {
            //No subscribers
            return false;
        }

        pNewSubscriberList = CSubscriberListPtr(new CSubscriberList());

        for (size_t i = 0; i < m_pSubscriberList->size(); ++i)
        {
            CSubscriberItemPtr pSubscriberItem = (*m_pSubscriberList)[i];

            if (pSubscriberItem->Subscriber()->GetSubscriberId() == id)
            {
                pSubscriberItem->ReqMenager()->removeReqIdsExt(_symbol, _reqData);
                if (0 == pSubscriberItem->ReqMenager()->getActiveRequestSize())
                {
                    pSubscriberItemToRelease = pSubscriberItem;
                }
                else
                {
                    pNewSubscriberList->push_back(pSubscriberItem);
                }

            }
            else
            {
                pNewSubscriberList->push_back(pSubscriberItem);
            }
        }

        //Exchange subscriber lists
        m_pSubscriberList = pNewSubscriberList;

        if (!pSubscriberItemToRelease.get())
        {
            return false;
        }
        return true;
    }

    //---------------------------------------------------------
    void CDispatcher::SendMessageToSubscribers(void* pContext, qint32 reqId, tEReqType reqDataId)
    {
        CSubscriberListPtr pSubscriberList;
        {
            //Enter to locked scope
            std::lock_guard<std::mutex> lock(g_i_mutex);
            if ((nullptr == m_pSubscriberList) || (m_pSubscriberList->empty()))
            {
                //No subscribers
                return;
            }
            //Get shared pointer to an existing list of subscribers
            pSubscriberList = m_pSubscriberList;
        }

        //pSubscriberList pointer to copy of subscribers' list
        for (size_t i = 0; i < pSubscriberList->size(); ++i)
        {
            if ((*pSubscriberList)[i]->ReqMenager()->isPresent(reqId, reqDataId) || (reqDataId == RT_NEXT_VALID_ID))
            {

                (*pSubscriberList)[i]->Subscriber()->MessageHandler(pContext, reqDataId);
            }
        }
    }

    bool CDispatcher::UnsubscribeAllItemsOfSubscriber(SubscriberId _SubscriberId)
    {
        bool res = true;
        CDispatcher::CSubscriberItemPtr pSubcrItem = getSubscriberItem(_SubscriberId);

        if (nullptr != pSubcrItem)
        {
            QList<stReqIds>  refListIds = pSubcrItem->ReqMenager()->getlistIDs();

            QList<stReqIds>::iterator it = refListIds.begin();
            for (; it != refListIds.end(); ++it)
            {
                if (it->id != 0)
                {
                    res = Unsubscribe(_SubscriberId, it->id, it->reqType);
                    pSubcrItem->ReqMenager()->removeReqIds(it->id, it->reqType);
                }
            }

            res |= Unsubscribe(_SubscriberId, -1, RT_HISTORICAL_DATA);
        }

        return res;
    }

    //-----------------------------------------------------
    bool CDispatcher::isReqIdEmpty(const QString _symbol, const stReqIds & _reqData)
    {
        quint32 itemsCount = 0;
        bool isEmpty = true;

        for (size_t i = 0; i < m_pSubscriberList->size(); ++i)
        {
            CSubscriberItemPtr pSubscriberItem = (*m_pSubscriberList)[i];

            itemsCount += pSubscriberItem->ReqMenager()->getItemsCountInTickerMap(_symbol, _reqData);
        }

        if (0 != itemsCount)
        {
            isEmpty = false;
        }

        return isEmpty;
    }


}
