
#include "cmomentum.h"

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Momentum";
    this->setName("Momentum");


    QObject::connect(this, &CProcessingBase_v2::signalCbkRecvHistoricalData, this, &cMomentum::slotCbkRecvHistoricalData, Qt::QueuedConnection);
}

bool cMomentum::start()
{
    //QList<QString> tickers = {"AAPL", "MSFT", "GOOG"};
    QList<QString> tickers = {"AAPL"};

    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "5 D", "");
    for (const QString& ticker : tickers) {
//        auto id = getNextValidId();
//        histConfiguration.id = id;
        histConfiguration.symbol = ticker.toStdString().c_str();
        reqestHistoricalData(histConfiguration);
    }

    return CBasicStrategy_V2::start();
}

bool cMomentum::stop()
{
    return CBasicStrategy_V2::stop();
}

void cMomentum::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCDebug(MomentumPmLog(), "symbol: [%s] - length: [%d]", _symbol.toStdString().c_str(), _histMap.length());
}

