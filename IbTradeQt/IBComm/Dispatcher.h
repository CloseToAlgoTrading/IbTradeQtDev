#ifndef Dispatcher_h
#define Dispatcher_h

#include <vector>
#include <memory>
#include <mutex>

#include "./ReqManager/globalreqmanager.h"
#pragma once
namespace Observer
{


	//////////////////////////
	// Subscriber (Observer)
	//////////////////////////

    //typedef unsigned __int64  SubscriberId;
    typedef quint64  SubscriberId;

        class CSubscriber
        {
	public:
		virtual ~CSubscriber(){}
		virtual void MessageHandler(void* pContext, tEReqType _reqType) = 0;
        virtual void UnsubscribeHandler() = 0;
        SubscriberId GetSubscriberId()   { return (SubscriberId)this; }
	};


	typedef CSubscriber* CSubscriberPtr;


	//////////////////////////////////////////////////////////////////////
	// Dispatcher (Observable)
	///////////////////////////////////
        class CDispatcher
        {
	private:
		
		class CSubscriberItem
		{
		public:
			CSubscriberItem(CSubscriberPtr pSubscriber)
				:m_pSubscriber(pSubscriber)
				, m_ReqMenager()
			{
				
			}
			~CSubscriberItem()
			{
				m_pSubscriber->UnsubscribeHandler();
                        }
			CSubscriberPtr Subscriber() const { return m_pSubscriber; }
			GlobalReqManager* ReqMenager() { return &m_ReqMenager; }
		private:
			CSubscriberPtr  m_pSubscriber;
			GlobalReqManager m_ReqMenager;

		};
		
	public:
		typedef std::shared_ptr<CSubscriberItem>  CSubscriberItemPtr;
		typedef std::vector<CSubscriberItemPtr>   CSubscriberList;
		typedef std::shared_ptr<CSubscriberList>  CSubscriberListPtr;
	
	public:
        CDispatcher() : m_pSubscriberList(new CSubscriberList()), g_i_mutex() {}
        ~CDispatcher(){};

	private:
		CDispatcher(const CDispatcher&){}
		CDispatcher& operator=(const CDispatcher&){ return *this; }

	public:

        const CSubscriberItemPtr getSubscriberItem(SubscriberId _subscriberId);

		//-------------------------------------------------------------
		//-------------------------------------------------------------
		//-------------------------------------------------------------
        //SubscriberId Subscribe(CSubscriberPtr pNewSubscriber, const qint32 reqId, const tEReqType reqDataId);
        SubscriberId Subscribe(CSubscriberPtr pNewSubscriber, const QString & _symbol, const stReqIds & _reqData);

		//-------------------------------------------------------------
		//-------------------------------------------------------------
		//-------------------------------------------------------------
        bool Unsubscribe(SubscriberId id, const qint32 reqId, const tEReqType reqDataId, const QString _s = "");
        bool Unsubscribe(SubscriberId id, const QString & _symbol, const stReqIds & _reqData);
		
		//-------------------------------------------------------------
		//-------------------------------------------------------------
		//-------------------------------------------------------------
        void SendMessageToSubscribers(void* pContext, qint32 reqId, tEReqType reqDataId);

        //-------------------------------------------------------------
        bool UnsubscribeAllItemsOfSubscriber(SubscriberId _SubscriberId);

        bool isReqIdEmpty(const QString _symbol, const stReqIds & _reqData);

    private:
		CSubscriberListPtr  m_pSubscriberList;
                std::mutex g_i_mutex;
        };

}; //namespace Observer

#endif // Dispatcher_h

