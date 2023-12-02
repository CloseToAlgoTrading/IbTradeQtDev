
#include "cmomentum.h"
#include "cbasicselectionmodel.h"
#include "cbasicalphamodel.h"
#include "UnifiedModelData.h"

#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Momentum";
    this->setName("Momentum");
}

bool cMomentum::start()
{
//    //QList<QString> tickers = {"AAPL", "MSFT", "GOOG"};
//    QList<QString> tickers = {"AAPL"};

//    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "5 D", "");
//    for (const QString& ticker : tickers) {
////        auto id = getNextValidId();
////        histConfiguration.id = id;
//        histConfiguration.symbol = ticker.toStdString().c_str();
//        reqestHistoricalData(histConfiguration);
//    }
    if(!isConnectedTotheServer()) return false;
    CBasicStrategy_V2::start();
    auto m_pAssetList = createDataList();
    emit dataProcessed(m_pAssetList);
    return true;
    //return CBasicStrategy_V2::start();
}

bool cMomentum::stop()
{
    return CBasicStrategy_V2::stop();
}


