// #include "pairtraderpm.h"
// #include "./IBComm/Dispatcher.h"
// #include <QSharedPointer>
// #include "contractsdefs.h"

// //#include "mkl.h"


// Q_LOGGING_CATEGORY(pairTraderPMLog, "pairTrader.PM");

// using namespace Observer;
// //----------------------------------------------------------
// PairTraderPM::PairTraderPM(QObject *parent, CBrokerDataProvider & _refClient)
//     : CProcessingBase(parent, _refClient)
//     , XIV_id(0)
// {
//     QObject::connect(this, &CProcessingBase::signalCbkRecvHistoricalData, this, &PairTraderPM::slotCbkRecvHistoricalData, Qt::AutoConnection);
// }

// //----------------------------------------------------------
// PairTraderPM::~PairTraderPM() = default;

// //----------------------------------------------------------
// //************************************
// // Method:    initStrategy
// // FullName:  PairTraderPM::initStrategy
// // Access:    public
// // Returns:   void
// // Qualifier:
// //
// // make your strategy initialization and configuration
// //************************************
// void PairTraderPM::initStrategy()
// {
//     reqestResetSubscription();

//     requestRealTimeBars("AMD");

//     reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "10 D", "XIV");

//     if (!reqestHistoricalData(histConfiguration))
//     {
//         qCWarning(pairTraderPMLog(), "request of historical data [XIV] failed!");
//     }
//     else
//     {
//         XIV_id = histConfiguration.id;
//     }
// }

// //************************************
// // Method:    recvHistoryOfXIV
// // FullName:  PairTraderPM::recvHistoryOfXIV
// // Access:    public
// // Returns:   void
// // Qualifier:
// //
// // do something with the history of XIV Data
// //************************************
// #define N 10
// #define DIM 1      /* dimension of the task */
// void PairTraderPM::recvHistoryOfXIV(const QList<IBDataTypes::CHistoricalData> & _histMap)
// {

//     qint32 a = _histMap.size();
//     double x[N]; /* Array for dataset */
//     double mean_my = 0.0;
//     qint32 i = 0;
//     foreach (IBDataTypes::CHistoricalData xivItem, _histMap)
//     {
//         x[i] = xivItem.getClose();
//         mean_my += x[i];

//         ++i;
//     }

//     mean_my /= N;

// //    VSLSSTaskPtr task; /* SS task descriptor */
// //    //double x[N]; /* Array for dataset */
// //    double mean; /* Array for mean estimate */
// //    double* w = nullptr; /* Null pointer to array of weights, default weight equal to one will be used in the computation */
// //    MKL_INT p, n, xstorage;
// //    int status;

// //    /* Initialize variables used in the computation of mean */
// //    p = DIM;
// //    n = N;
// //    xstorage = VSL_SS_MATRIX_STORAGE_ROWS;
// //    mean = 0.0;

// //    /* Step 1 - Create task */
// //    status = vsldSSNewTask(&task, &p, &n, &xstorage, x, w, 0);
// //    /* Step 2- Initialize task parameters */
// //    status = vsldSSEditTask(task, VSL_SS_ED_MEAN, &mean);
// //    /* Step 3 - Compute the mean estimate using SS one-pass method */
// //    status = vsldSSCompute(task, VSL_SS_MEAN, VSL_SS_METHOD_1PASS);
// //    /* Step 4 - deallocate task resources */
// //    status = vslSSDeleteTask(&task);

// //    qCDebug(pairTraderPMLog(), "my_mean = %f, intel mean = %f", mean_my, mean);

// }

// //----------------------------------------------------------


// //----------------------------------------------------------
// void PairTraderPM::slotOnAddNewPair(QString _s1, QString _s2)
// {

//     reqReadlTimeDataConfigData_t s1Cfg{0, ContractsDefs::USStock(_s1), "", false, false};
//     reqReadlTimeDataConfigData_t s2Cfg{0, ContractsDefs::USStock(_s2), "", false, false};
//     reqestRealTimeData(s1Cfg);
//     reqestRealTimeData(s2Cfg);

//     //initStrategy();
// }


// //----------------------------------------------------------
// void PairTraderPM::slotOnRemoveSymbol(const QString & s1)
// {
//     cancelRealTimeData(s1);
// }

// //----------------------------------------------------------
// void PairTraderPM::slotOnRequestHistory(const QString & s1)
// {
//     reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_HOUR, "5 D", s1);

//     if (!reqestHistoricalData(histConfiguration))
//     {
//         qCWarning(pairTraderPMLog(), "request of historical data [%s] failed!", s1.toLocal8Bit().data());
//     }
// }

// //----------------------------------------------------------
// void PairTraderPM::slotOnGUIClosed()
// {
//     cancelAllActiveRequests();
//     UnsubscribeHandler();
// }

// //----------------------------------------------------------
// void PairTraderPM::slotOnRequestSendBuyMkt(const QString& _symbol, const quint32 _quantity)
// {
//     requestPlaceMarketOrder(_symbol, _quantity, OA_BUY);
//    // requestOpenOrders();
// }

// //----------------------------------------------------------
// void PairTraderPM::slotOnRequestSendSellMkt(const QString& _symbol, const quint32 _quantity)
// {
//     requestPlaceMarketOrder(_symbol, _quantity, OA_SELL);
// }

// //----------------------------------------------------------
// void PairTraderPM::slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol)
// {
//     emit signalOnFinishHistoricalData(_histMap, _symbol);

//     //---- strategy impl ---
//     if (_symbol == "XIV")
//     {
//         recvHistoryOfXIV(_histMap);
//     }
    
// }

// //----------------------------------------------------------
// void PairTraderPM::callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol)
// {
//     emit signalOnRealTimeTickData(_tickPrize, _symbol);
// }

// //----------------------------------------------------------
// void PairTraderPM::calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol)
// {
//    // signalOnFinishHistoricalData(_histMap, _symbol);
// }

