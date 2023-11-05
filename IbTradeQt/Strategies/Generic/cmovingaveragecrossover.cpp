#include "cmovingaveragecrossover.h"
#include "IBComClientImpl.h"
#include "contractsdefs.h"
#include "Decimal.h"

Q_LOGGING_CATEGORY(MaCrossoverPmLog, "MovingAverage.PM");

CMovingAverageCrossover::CMovingAverageCrossover(QObject *parent) : CBasicStrategy_V2(parent)
{
    //TODO: The strategy must have access to the BrokerInbterface!

//    QSharedPointer<IBComClientImpl> pClient = QSharedPointer<IBComClientImpl>::create(m_DataProvider);
//    m_DataProvider.setClien(pClient);

//    QSharedPointer<CBrokerDataProvider> dataProviderPtr = QSharedPointer<CBrokerDataProvider>::create(m_DataProvider);


//    this->setClient(dataProviderPtr);
    //m_DataProvider.setClien(QSharedPointer<IBComClientImpl>::create(m_DataProvider));
    //this->setClient(QSharedPointer<CBrokerDataProvider>(&m_DataProvider));
    /***** Init Patameters ******/
    m_Name = "Moving Average Crossover";
    this->setName("Moving Average Crossover");

    this->m_ParametersMap["Asset"] = "SPY";
    this->m_ParametersMap["MA_Fast"] = 5;
    this->m_ParametersMap["MA_Slow"] = 15;

    QObject::connect(this, &CProcessingBase_v2::signalEndRecvPosition, this, &CMovingAverageCrossover::slotEndRecvPosition, Qt::QueuedConnection);
}

bool CMovingAverageCrossover::start()
{
//    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "10 D", "SPY");

//    if (!reqestHistoricalData(histConfiguration))
//    {
//        qCWarning(MaCrossoverPmLog(), "request of historical data [SPY] failed!");
//    }
//    QString m_TicketName = "EUR";
//    //Contract optContract = ContractsDefs::CashCFD(m_TicketName);
//    Contract contract;
//    contract.symbol = "EUR";
//    contract.secType = "CASH";
//    contract.currency = "USD";
//    contract.exchange = "IDEALPRO";

//    reqReadlTimeDataConfigData_t optCfg{0, contract, "", false, false};
//    reqestRealTimeData(optCfg);

    requestPosition();

    return CBasicStrategy_V2::start();
}

bool CMovingAverageCrossover::stop()
{
    return CBasicStrategy_V2::stop();
}

void CMovingAverageCrossover::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    Q_UNUSED(_histMap);
    qCDebug(MaCrossoverPmLog(), "Historical data receiverd [%s]", _symbol.toStdString().c_str());
}

void CMovingAverageCrossover::slotEndRecvPosition()
{
    qCDebug(MaCrossoverPmLog(), "TADA!!!");
    m_assetList.clear();
    // find all stock position according to ticketName
    for (auto iter = m_positionMap.constBegin(); iter != m_positionMap.constEnd(); ++iter)
    {
        if(iter.value().getPos() > 0.0)
        {
            qCDebug(MaCrossoverPmLog(), "Ticker [%s] size[%f] cost[%f]", iter.key().toStdString().c_str(), iter.value().getPos(), iter.value().getAvgCost());
            this->m_assetList[iter.key()] = QVariantMap({{"Size", iter.value().getPos()}, {"Avg. Cost",iter.value().getAvgCost()}});
        }

    }



}
