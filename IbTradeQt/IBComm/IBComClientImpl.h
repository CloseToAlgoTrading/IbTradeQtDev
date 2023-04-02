#pragma once

#ifndef IBComClientImpl_H_INCLUDED
#define IBComClientImpl_H_INCLUDED

#include "Dispatcher.h"

#include "Contract.h"
#include "MyLogger.h"
#include "IBrokerAPI.h"

#include "EWrapper.h"
#include "EReader.h"
#include <QSharedPointer>
//#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(IBComClientImplLog);

class EPosixClientSocket;

class IBComClientImpl : public EWrapper, public IBrokerAPI
{

public:
    qint32 getNexValidId() { return m_nexValidId++; }

    IBComClientImpl(Observer::CDispatcher & _dispatcher);
    ~IBComClientImpl() override;

	//void setUseV100Plus(const std::string&);

    void setConnectOptions(const std::string& connectOptions);
    void processMessagesAPI() override;

public:
	//IBrokerAPI
    bool connectAPI(const char *host, unsigned int port, int clientId) override;
    bool isConnectedAPI() override;
    void disconnectAPI() override;


	/***************************************************************
	* \brief	Request current time from server
	* \param[in]	subscrId	Subscriber Id
	* \returns	None
	****************************************************************/
    void reqCurrentTimeAPI() override;
	
	/***************************************************************
	* \brief	Request real time data from server
	* \param[in]	symbol		requested symbol (Example: "AAPL")
	* \returns	None
	****************************************************************/
    void reqRealTimeDataAPI(const qint32 _id, reqReadlTimeDataConfigData_t &_config) override;

	/***************************************************************
	* \brief	Request current time from server
	* \returns	None
	****************************************************************/
    void cancelRealTimeDataAPI(const qint32 _id) override;

	/***************************************************************
	* \brief	Request current time from server
	* \param[in]	symbol		requested symbol (Example: "AAPL")
	* \returns	None
	****************************************************************/
    void reqHistoricalDataAPI(const reqHistConfigData_t & _config) override;

	/***************************************************************
	* \brief	Request current time from server
	* \param[in]	subscrId	request Id
	* \returns	None
	****************************************************************/
    void cancelHistoricalDataAPI(const qint32 id) override;

    //************************************
    // Method:    reqRealTimeBars
    // FullName:  IBComClientImpl::reqRealTimeBars
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const qint32 _id
    // Parameter: const reqlTimeBarsReq_t & _reqRTBar
    //************************************
    void reqRealTimeBarsAPI(const qint32 _id, const QString& _symbol) override;

    //************************************
    // Method:    cancelRealTimeBars
    // FullName:  IBComClientImpl::cancelRealTimeBars
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: qint32 _id
    //************************************
    void cancelRealTimeBarsAPI(const qint32 _id) override;

    ///position
    //************************************
    // Method:    reqPositionAPI
    // FullName:  IBComClientImpl::reqPositionAPI
    // Access:    public
    // Returns:   void
    // Qualifier:
    // Parameter: const qint32 _id
    //************************************
    void reqPositionAPI(const qint32 _id) override;

    //************************************
    // Method:    cancelPositionAPI
    // FullName:  IBComClientImpl::cancelPositionAPI
    // Access:    public
    // Returns:   void
    // Qualifier:
    // Parameter: qint32 _id
    //************************************
    void cancelPositionAPI(const qint32 _id) override;

    ///orders
    //************************************
    // Method:    reqPlaceOrderAPI
    // FullName:  IBComClientImpl::reqPlaceOrderAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const qint32 _id
    // Parameter: const reqPlaceOrder_t & _reqOrder
    //************************************
    qint32 reqPlaceOrderAPI(const QString& _symbol, const qint32 _quantity, const orderAction _action) override;


    //************************************
    // Method:    cancelOrderAPI
    // FullName:  IBComClientImpl::cancelOrderAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const qint32 _id
    //************************************
    void cancelOrderAPI(const qint32 _id) override;

    //************************************
    // Method:    reqOpenOrdersAPI
    // FullName:  IBComClientImpl::reqOpenOrdersAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    //************************************
    void reqOpenOrdersAPI() override;

    //************************************
    // Method:    reqAllOpenOrdersAPI
    // FullName:  IBComClientImpl::reqAllOpenOrdersAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    //************************************
    void reqAllOpenOrdersAPI() override;

    //************************************
    // Method:    reqAutoOpenOrdersAPI
    // FullName:  IBComClientImpl::reqAutoOpenOrdersAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const bool _bAutoBind
    //************************************
    void reqAutoOpenOrdersAPI(const bool _bAutoBind) override;

    //************************************
    // Method:    reqNextValidIDsAPI
    // FullName:  IBComClientImpl::reqNextValidIDsAPI
    // Access:    public 
    // Returns:   void
    // Qualifier:
    // Parameter: const qint32 _numIds
    //************************************
    void reqNextValidIDsAPI(const qint32 _numIds) override;

    //************************************
    // Method:    reqGlobalCancel
    // FullName:  IBComClientImpl::reqGlobalCancel
    // Access:    public 
    // Returns:   void
    // Qualifier:
    //************************************
    void reqGlobalCancelAPI() override;


    void reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t & _config) override;
    void cancelCalculateOptionPriceAPI(const qint32 _id) override;

    void reqHistoricalTicksAPI(const reqHistTicksConfigData_t & _config) override;

    void reqTickByTickDataAPI(const reqTickByTickDataConfigData_t & _config) override;
    void cancelTickByTickDataAPI(const qint32 id) override;

public:
	// events
    void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;
    void tickSize( TickerId tickerId, TickType field, Decimal size) override;
    void tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
                                double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) override;
    void tickGeneric(TickerId tickerId, TickType tickType, double value) override;
    void tickString(TickerId tickerId, TickType tickType, const std::string& value) override;
    void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
        double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) override;
    void orderStatus( OrderId orderId, const std::string& status, Decimal filled,
        Decimal remaining, double avgFillPrice, int permId, int parentId,
        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) override;
    void openOrder( OrderId orderId, const Contract&, const Order&, const OrderState&) override;
    void openOrderEnd() override;
    void winError( const std::string& str, int lastError) override;
    void connectionClosed() override {}
    void updateAccountValue(const std::string& key, const std::string& val,
                            const std::string& currency, const std::string& accountName) override;
    void updatePortfolio( const Contract& contract, Decimal position,
        double marketPrice, double marketValue, double averageCost,
        double unrealizedPNL, double realizedPNL, const std::string& accountName) override {}
    void updateAccountTime(const std::string& timeStamp) override {}
    void accountDownloadEnd(const std::string& accountName) override {}
    void nextValidId( OrderId orderId) override;
    void contractDetails( int reqId, const ContractDetails& contractDetails) override {}
    void bondContractDetails( int reqId, const ContractDetails& contractDetails) override {}
    void contractDetailsEnd( int reqId) override {}
    void execDetails( int reqId, const Contract& contract, const Execution& execution) override;
    void execDetailsEnd( int reqId) override;
    void error(int id, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) override;
    void updateMktDepth(TickerId id, int position, int operation, int side,
        double price, Decimal size) override;
    void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
        int side, double price, Decimal size, bool isSmartDepth) override;
    void updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) override {}
    void managedAccounts( const std::string& accountsList) override {}
    void receiveFA(faDataType pFaDataType, const std::string& cxml) override {}
    void historicalData(TickerId reqId, const Bar& bar) override;
    void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) override {}
    void scannerParameters(const std::string& xml) override {}
    void scannerData(int reqId, int rank, const ContractDetails& contractDetails,
        const std::string& distance, const std::string& benchmark, const std::string& projection,
        const std::string& legsStr) override {}
    void scannerDataEnd(int reqId) override {}
    void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
        Decimal volume, Decimal wap, int count) override;
    void currentTime(long time) override;
    void fundamentalData(TickerId reqId, const std::string& data) override {}
    void deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) override {}
    void tickSnapshotEnd( int reqId) override {}
    void marketDataType( TickerId reqId, int marketDataType) override {}
    void commissionReport( const CommissionReport& commissionReport) override;
    void position( const std::string& account, const Contract& contract, Decimal position, double avgCost) override;
    void positionEnd() override;
    void accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& curency) override {}
    void accountSummaryEnd( int reqId) override {}
    void verifyMessageAPI( const std::string& apiData) override {}
    void verifyCompleted( bool isSuccessful, const std::string& errorText) override {}
    void displayGroupList( int reqId, const std::string& groups) override {}
    void displayGroupUpdated( int reqId, const std::string& contractInfo) override {}
    void verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange) override {}
    void verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) override {}
    void connectAck() override;
    void positionMulti( int reqId, const std::string& account,const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost) override {}
    void positionMultiEnd( int reqId) override {}
    void accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) override {}
    void accountUpdateMultiEnd( int reqId) override {}
    void securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
        const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) override {}
    void securityDefinitionOptionalParameterEnd(int reqId) override {}
    void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) override {}
    void familyCodes(const std::vector<FamilyCode> &familyCodes) override {}
    void symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) override {}
    void mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) override {}
    void tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) override {}
    void smartComponents(int reqId, const SmartComponentsMap& theMap) override {}
    void tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) override {}
    void newsProviders(const std::vector<NewsProvider> &newsProviders) override {}
    void newsArticle(int requestId, int articleType, const std::string& articleText) override {}
    void historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) override {}
    void historicalNewsEnd(int requestId, bool hasMore) override {}
    void headTimestamp(int reqId, const std::string& headTimestamp) override {}
    void histogramData(int reqId, const HistogramDataVector& data) override {}
    void historicalDataUpdate(TickerId reqId, const Bar& bar) override {}
    void rerouteMktDataReq(int reqId, int conid, const std::string& exchange) override {}
    void rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) override {}
    void marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) override {}
    void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) override {}
    void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) override {}
    void historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done) override {}
    void historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done) override {}
    void historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done) override;
    void tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) override;
    void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override {}
    void tickByTickMidPoint(int reqId, time_t time, double midPoint) override {}
    void orderBound(long long orderId, int apiClientId, int apiOrderId) override {}
    void completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) override {}
    void completedOrdersEnd() override {}
    void replaceFAEnd(int reqId, const std::string& text) override {}
    void wshMetaData(int reqId, const std::string& dataJson) override {}
    void wshEventData(int reqId, const std::string& dataJson) override {}
    void historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime, const std::string& timeZone, const std::vector<HistoricalSession>& sessions) override {}
    void userInfo(int reqId, const std::string& whiteBrandingId) override {}
private:

	/** IB socket client */
    //! [socket_declare]
    EReaderOSSignal m_osSignal;
    //QSharedPointer<EClientSocket> m_pClient;
    EClientSocket * const m_pClient;

    EReader *m_pReader;
    bool m_extraAuth;

	/** Logger */
	MyLogger& m_pLog;

	/** request manager */
    //ReqManager reqManager;

    qint32 m_nexValidId;

    Observer::CDispatcher & m_DispatcherBrokerData;

};

#endif // IBComClientImpl_H_INCLUDED
