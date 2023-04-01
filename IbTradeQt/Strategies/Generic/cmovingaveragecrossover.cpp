#include "cmovingaveragecrossover.h"

Q_LOGGING_CATEGORY(MaCrossoverPmLog, "MovingAverage.PM");

CMovingAverageCrossover::CMovingAverageCrossover(QObject *parent) : CProcessingBase_v2(parent)
  , CBasicStrategy()

{
    this->setClient(QSharedPointer<CBrokerDataProvider>(&m_DataProvider));
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

    return CBasicStrategy::start();
}

bool CMovingAverageCrossover::stop()
{
    return CBasicStrategy::stop();
}

void CMovingAverageCrossover::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCDebug(MaCrossoverPmLog(), "Historical data receiverd [%s]", _symbol.toStdString().c_str());
}
