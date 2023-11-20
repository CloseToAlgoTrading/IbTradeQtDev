
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


    QObject::connect(this, &CProcessingBase_v2::signalCbkRecvHistoricalData, this, &cMomentum::slotCbkRecvHistoricalData, Qt::QueuedConnection);
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
    CBasicStrategy_V2::start();
    return m_SelectionModel->start();
    //return CBasicStrategy_V2::start();
}

bool cMomentum::stop()
{
    return CBasicStrategy_V2::stop();
}

bool cMomentum::executeModels()
{
    QStringList assets;
    auto p_data = createDataList();

    if(nullptr != this->m_SelectionModel)
    {
        QSharedPointer<CBasicSelectionModel> derivedPointer = m_SelectionModel.staticCast<CBasicSelectionModel>();
    }
    /* check the type of subscription and if the assets are changed */


    if(nullptr != this->m_AlphaModel)
    {
        QSharedPointer<CBasicAlphaModel> derivedPointer = m_AlphaModel.staticCast<CBasicAlphaModel>();
    }
    return false;
}

void cMomentum::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCDebug(MomentumPmLog(), "symbol: [%s] - length: [%d]", _symbol.toStdString().c_str(), _histMap.length());
}

