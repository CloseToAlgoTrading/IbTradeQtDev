#ifndef CPROCESSINGBASE_V2_H
#define CPROCESSINGBASE_V2_H

#include <QObject>
#include <memory>

#include "CHistoricalData.h"
#include "caccountsummary.h"
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
#include "cexecutionreport.h"
#include "ccommissionreport.h"


using namespace IBDataTypes;

Q_DECLARE_LOGGING_CATEGORY(processingBaseV2Log);

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

namespace IBComm { class ProcessingRouterSink; }

class CProcessingBase_v2 : public QObject
{
	Q_OBJECT

    friend class IBComm::ProcessingRouterSink;

public:
    explicit CProcessingBase_v2(QObject *parent);
    virtual ~CProcessingBase_v2();

    qint32 getNextValidId() const { return m_nextValidId; }
    void setNextValidId(const qint32 val) { m_nextValidId = val; }

private:
    QSharedPointer<CBrokerDataProvider> m_Client;
    std::unique_ptr<IBComm::ProcessingRouterSink> m_routerSink;
    ActiveReqestsMap_t m_aciveReqestsMap;

public:
    HistoricalDataMap_t m_historyMap;

	HistMap_t			m_histMap;
	TickPriceMap_t		m_tickPriceMap;
	TickSizeMap_t		m_tickSizeMap;
	RealTimeBarMap_t	m_realTImeBarMap;
	MKDepthMap_t		m_mkDepthMap;
    PositionMap_t       m_positionMap;

    qint32 m_nextValidId;

    qint32 getRequestMapSize() const;

	bool reqestHistoricalData(reqHistConfigData_t & _config);
    bool requestHistoricalTicksData(reqHistTicksConfigData_t & _config);

    bool reqestRealTimeData(reqReadlTimeDataConfigData_t & _config);
	bool cancelRealTimeData(const QString& _symbol);

    bool requestRealTimeBars(const QString& _symbol);
    bool cancelRealTimeBars(const QString& _symbol);

    bool pbRequestPosition();
    bool pbCancelPosition();

    bool reqestResetSubscription();
    bool cancelResetSubscription();

    bool reqestErrorNotificationSubscription();
    bool cancelErrorNotificationSubscription();

    bool reqestOrderStatusSubscription();
    bool cancelOrderStatusSubscription();

    bool requestTickByTickData(reqTickByTickDataConfigData_t & _config);
    bool cancelTickByTickData(const QString& _symbol);

    bool requestCalculateOptionPrice(reqCalcOptPriceConfigData_t & _config);
    bool cancelCalculateOptionPrice(const QString& _symbol);

    bool pbReqAccountSummary();
    bool pbCancelAccountSummary();

    qint32 requestPlaceMarketOrder(const QString& _symbol, const qint32 _quantity, const eOrderAction_t _action);
    void requestOpenOrders();

    void cancelAllActiveRequests();
    bool isConnectedTotheServer();

    virtual void callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol);
    virtual void calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
    virtual void callback_recvPositionEnd();

    QSharedPointer<CBrokerDataProvider> getIBrokerDataProvider() const;
    void setIBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient);

signals:
    void signalCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);
    void signalRecvOptionTickComputation(const COptionTickComputation & obj);
    void signalEndRecvPosition();
    void signalRecvCommissionReport(const CCommissionReport & obj);
    void signalRecvExecutionReport(const CExecutionReport & obj);
    void signalRestartSubscription();
    void signalErrorNotFound(int id);
    void signalRecvAccountSummary(const CAccountSummary & obj);
    void signalTest();
};

#endif // CPROCESSINGBASE_V2_H
