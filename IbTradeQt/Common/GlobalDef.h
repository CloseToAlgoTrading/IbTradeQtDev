#ifndef GLOBALDEF_H
#define GLOBALDEF_H

//#define DEBUG_ACTIVE

#include "EWrapper.h"
#include "Contract.h"
#include "Order.h"
#include <QString>

#define APP_VERSION "0.2.0"


#define CONNECTIONS_SERVER_IP "127.0.0.1"
#define CONNECTIONS_REAL_SERVER_PORT 7496
#define CONNECTIONS_DEMO_SERVER_PORT 7497
#define CONNECTIONS_DEMO_GW_SERVER_PORT 4002
#define CONNECTIONS_CLIENT_ID 1

//#define CONNECTIONS_SERVER_PORT CONNECTIONS_REAL_SERVER_PORT
//#define CONNECTIONS_SERVER_PORT CONNECTIONS_DEMO_SERVER_PORT
#define CONNECTIONS_SERVER_PORT CONNECTIONS_DEMO_GW_SERVER_PORT



struct  reqHist_t
{
    // Defines a query end date and time at any point during the past 6 mos.
    // Valid values include any date / time within the past six months in the format : yyyymmdd HH : mm : ss ttt where "ttt" is the optional time zone.
	QString strEndDate; 
	// Amount of time up to the end date
    // Set the query duration up to one week, using a time unit of seconds, days or weeks.
    // Valid values include any integer followed by a space and then S(seconds), D(days) or W(week).If no unit is specified, seconds is used.
	QString strDuration;
    //Specifies the size of the bars that will be returned(within IB / TWS limits).Valid values include :
    //1 sec, 5 secs, 15 secs, 30 secs, 1 min, 2 mins, 3 mins, 5 mins, 15 mins, 30 mins, 1 hour, 1 day
	QString strBarSize = "1 hour";
    // Determines the nature of data being extracted.Valid values include :
    // TRADES, MIDPOINT, BID, ASK, BID_ASK, HISTORICAL_VOLATILITY, OPTION_IMPLIED_VOLATILITY
	QString strWhatToShow = "TRADES";
    // This object contains a description of the contract for which market data is being requested.
	Contract m_contract;
};

struct reqRealTimeBars_t
{
    // 	This object contains a description of the contract for which real time bars are being requested
    Contract contract;
    // Currently only 5 second bars are supported, if any other value is used, an exception will be thrown. 
    const qint32 barSize = 5;
    //Determines the nature of the data extracted.Valid values include :
    //  TRADES, BID, ASK, MIDPOINT
    QString whatToShow = "TRADES";
    //Regular Trading Hours only.Valid values include :
    //  0 = all data available during the time span requested is returned, including time intervals when the market in question was outside of regular trading hours.
    //  1 = only data within the regular trading hours for the product requested is returned, even if the time time span falls partially or completely outside
    bool useRTH = true;
};


struct reqPlaceOrder_t
{
    // This structure contains a description of the contract which is being traded.
    Contract contract;
    // This structure contains the details of the order.Note: Each client MUST connect with a unique clientId.
    Order order;
};


/************************************************************************/
#define BAR_SIZE_1_SEC  "1 sec"
#define BAR_SIZE_5_SECS  "5 secs"
#define BAR_SIZE_10_SECS "10 secs"
#define BAR_SIZE_15_SECS "15 secs"
#define BAR_SIZE_30_SECS "30 secs"

#define BAR_SIZE_1_MIN  "1 min"
#define BAR_SIZE_2_MINS  "2 mins"
#define BAR_SIZE_10_MINS "10 mins"
#define BAR_SIZE_15_MINS "15 mins"
#define BAR_SIZE_20_MINS "20 mins"
#define BAR_SIZE_30_MINS "30 mins"

#define BAR_SIZE_1_HOUR  "1 hour"
#define BAR_SIZE_2_HOURS  "2 hours"
#define BAR_SIZE_3_HOURS  "3 hours"
#define BAR_SIZE_4_HOURS  "4 hours"
#define BAR_SIZE_8_HOURS  "8 hours"

#define BAR_SIZE_1_DAY   "1 day"

#define BAR_SIZE_1_WEEK  "1 week"

#define BAR_SIZE_1_MONTH "1 month"


enum eBarSize_t
{
    E_BAR_SIZE_1_SEC,
    E_BAR_SIZE_5_SEC,
    E_BAR_SIZE_10_SEC,
    E_BAR_SIZE_15_SEC,
    E_BAR_SIZE_30_SEC,

    E_BAR_SIZE_1_MIN,
    E_BAR_SIZE_2_MIN,
    E_BAR_SIZE_10_MIN,
    E_BAR_SIZE_15_MIN,
    E_BAR_SIZE_20_MIN,
    E_BAR_SIZE_30_MIN,

    E_BAR_SIZE_1_HOUR,
    E_BAR_SIZE_2_HOUR,
    E_BAR_SIZE_3_HOUR,
    E_BAR_SIZE_4_HOUR,
    E_BAR_SIZE_8_HOUR,

    E_BAR_SIZE_1_DAY,
    E_BAR_SIZE_1_WEEK,
    E_BAR_SIZE_1_MONT
};


struct reqHistConfigData_t
{
    reqHistConfigData_t(const qint32 _id, const QString _barsize, const QString _duration, const QString _symbol)
        : id(_id)
        , barSize(_barsize)
        , duration(_duration)
        , symbol(_symbol)
    {}

    qint32  id;
    QString barSize;
    QString duration;
    QString symbol;
};


struct reqHistTicksConfigData_t
{
    qint32  id;
    QString symbol;
    Contract contract;
    QString	startDateTime;
    QString	endDateTime;
    qint32 	numberOfTicks;
    QString	whatToShow;
    qint32 	useRth;
    bool 	ignoreSize;
    //QList< TagValue > miscOptions;

};

struct reqReadlTimeDataConfigData_t
{
    qint32  id;
    Contract contract;
    QString	genericTickList;
    bool 	snapshot;
    bool 	regulatorySnaphsot;
    //QList< TagValue > mktDataOptions;
};

struct reqTickByTickDataConfigData_t
{
    qint32  id;
    Contract contract;
    QString	tickType;
    qint32 	numberOfTicks;
    bool 	ignoreSize;
};


struct reqCalcImplVolConfigData_t
{
    qint32  id;
    QString symbol;
    Contract contract;
    qreal optionPrice;
    qreal underPrice;
    //QList< TagValue > impliedVolatilityOptions ;
};

struct reqCalcOptPriceConfigData_t
{
    qint32  id;
    QString symbol;
    Contract contract;
    qreal volatility;
    qreal underPrice;
    //QList< TagValue > optionPriceOptions ;
};


//Q_GLOBAL_STATIC(QString, TimeSymbol, QStringLiteral("Time"));
const QString TimeSymbol = "Time";
const QString PositionSymbol = "Position";
const QString RestartRequestSymbol = "Restart";
const QString OrderStatusSymbol = "OrderStatus";
const QString ErrorSymbol = "Error";


/************************************************************************/
#endif //GLOBALDEF_H
