#include "cmovingaveragecrossover.h"
#include "IBComClientImpl.h"

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
    this->setName("Moving Average Crossover");

    this->m_ParametersMap["Asset"] = "SPY";
    this->m_ParametersMap["MA_Fast"] = 5;
    this->m_ParametersMap["MA_Slow"] = 15;
}

bool CMovingAverageCrossover::start()
{
    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "10 D", "SPY");

    if (!reqestHistoricalData(histConfiguration))
    {
        qCWarning(MaCrossoverPmLog(), "request of historical data [SPY] failed!");
    }

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
