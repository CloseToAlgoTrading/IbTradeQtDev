#ifndef CPROCESSINGBASE_H
#define CPROCESSINGBASE_H

#include <QObject>
#include "Dispatcher.h"

#include "CHistoricalData.h"
#include "ctickprice.h"
#include "cticksize.h"
#include "crealtimebar.h"
#include "cmktdepth.h"
#include "cposition.h"
#include "coptiontickcomputation.h"
#include "./ReqManager/globalreqmanager.h"
#include "./IBComm/cbrokerdataprovider.h"
#include <QLoggingCategory>
#include <QList>
#include "GlobalDef.h"


using namespace IBDataTypes;

Q_DECLARE_LOGGING_CATEGORY(processingBaseLog);

typedef struct 
{
    QList<CHistoricalData> listHistData;
    bool isAvaliable;
} HistoricalData_st;

typedef QMap<qint64, HistoricalData_st>	HistoricalDataMap_t;

typedef QMultiMap<QString, tEReqType>	ActiveReqestsMap_t;

typedef QMultiMap<qint64, CHistoricalData>	HistMap_t;
typedef QMultiMap<qint64, IBDataTypes::CMyTickPrice> TickPriceMap_t;
typedef QMultiMap<qint64, CTickSize>		TickSizeMap_t;
typedef QMultiMap<qint64, CrealtimeBar>		RealTimeBarMap_t;
typedef QMultiMap<qint64, CMktDepth>		MKDepthMap_t;

typedef QMultiMap<QString, CPosition>	    PositionMap_t;

Q_DECLARE_METATYPE(HistMap_t);


class CProcessingBase : public QObject, public Observer::CSubscriber
{
	Q_OBJECT

public:
    explicit CProcessingBase(QObject *parent, CBrokerDataProvider & _refClient);
    virtual ~CProcessingBase();

	void MessageHandler(void* pContext, tEReqType _reqType);
	void UnsubscribeHandler();

    qint32 getNextValidId() const { return m_nextValidId; }
    void setNextValidId(const qint32 val) { m_nextValidId = val; }
private:
    void recvHistoricalData(void* pContext, tEReqType _reqType);
    void recvTickSize(void* pContext, tEReqType _reqType);
    void recvTickPrize(void* pContext, tEReqType _reqType);
    virtual void recvRealtimeBar(void* pContext, tEReqType _reqType);
    virtual void recvTickByTickAllLastData(void* pContext, tEReqType _reqType);
    void recvMktDepth(void* pContext, tEReqType _reqType);
    void recvPosition(void* pContext, tEReqType _reqType);
    void recvPositionEnd();
    void recvOrdersCommission(void* pContext, tEReqType _reqType);

    void recvOptionTickComputation(void* pContext, tEReqType _reqType);

    virtual void recvRestartSubscription();


    //temporary here
    bool calculateOneMinBar(RealTimeBarMap_t & _realTimeBar, TickerId _id);


//signals:

    //Todo: implement??
    //void signalOnRealTimeTickDataBase(const IBDataTypes::CMyTickPrice & _pT, const QString _s2);
    //void signalOnFinishHistoricalDataBase(const QList<CHistoricalData> & _pT, const QString _s1);


private:
	
    CBrokerDataProvider & m_Client;
    ActiveReqestsMap_t m_aciveReqestsMap;

public:
	//members

    HistoricalDataMap_t m_historyMap;

	HistMap_t			m_histMap;
	TickPriceMap_t		m_tickPriceMap;
	TickSizeMap_t		m_tickSizeMap;
	RealTimeBarMap_t	m_realTImeBarMap;
	MKDepthMap_t		m_mkDepthMap;
    PositionMap_t       m_positionMap;

    //nextValidID
    qint32 m_nextValidId;

    qint32 getRequestMapSize() const;

	//functions realized requests to data provider
	bool reqestHistoricalData(reqHistConfigData_t & _config);

    bool requestHistoricalTicksData(reqHistTicksConfigData_t & _config);

    bool reqestRealTimeData(reqReadlTimeDataConfigData_t & _config);
	bool cancelRealTimeData(const QString& _symbol);

    bool requestRealTimeBars(const QString& _symbol);
    bool cancelRealTimeBars(const QString& _symbol);

    bool requestPosition();
    bool cancelPosition();

    bool reqestResetSubscription();
    bool cancelResetSubscription();

    bool reqestOrderStatusSubscription();
    bool cancelOrderStatusSubscription();

    bool requestTickByTickData(reqTickByTickDataConfigData_t & _config);
    bool cancelTickByTickData(const QString& _symbol);

    bool requestCalculateOptionPrice(reqCalcOptPriceConfigData_t & _config);
    bool cancelCalculateOptionPrice(const QString& _symbol);
    //orders
    qint32 requestPlaceMarketOrder(const QString& _symbol, const qint32 _quantity, const orderAction _action);

    void requestOpenOrders();

    void cancelAllActiveRequests();

    bool isConnectedTotheServer();


	//helper functions
    const QString getSymbolFromRM(const qint64 _Id, const tEReqType _reqType = RT_REQ_REL_DATA);
    qint64 getIdFromRM(const QString _symbol, const tEReqType _reqType = RT_REQ_REL_DATA);

    void removeOldHistoricalRequest(const QString & _s1);

    void removeOldHistoricalTickRequest(const QString & _s1);

    virtual void callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol);
    virtual void calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
    virtual void callback_recvPositionEnd();

//public slots:
//    virtual void slotMessageHandler(void* pContext, tEReqType _reqType);
    
signals:
    void signalCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
    void signalMessageHandler(void* pContext, tEReqType _reqType);
    void signalRecvOptionTickComputation(const COptionTickComputation & obj);
    void signalEndRecvPosition();
    void signalRecvOrderCommission(const qreal & obj, const qreal & _rpnl);
    void signalRestartSubscription();

//    void signalRecvRealTimeBar(const RealTimeBarMap_t & _realTimeBar, const TickerId & _id);

    void signalTest();

};

#endif // CPROCESSINGBASE_H
