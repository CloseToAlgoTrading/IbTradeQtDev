
#include "cbasicalphamodel.h"
//#include <QDateTime>
#include <QStringList>

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_historicalData()
    , m_pProcessingData()
    , m_receivedDataCounter(0)
{
    m_Name = "Base Alpha Model";
    this->setName("Base Alpha Model");

    QObject::connect(this, &CProcessingBase_v2::signalCbkRecvHistoricalData, this, &CBasicAlphaModel::slotCbkRecvHistoricalData, Qt::QueuedConnection);
}

void CBasicAlphaModel::processData(DataListPtr data)
{
    if(nullptr == data) return;

    auto item = data->constLast();
    qCDebug(BasicAlphaModelLog(), "data receveid %lld %s %f", data->length(), item.symbol.toStdString().c_str(), item.amount);

    m_pProcessingData = data;

    QStringList tickers;
    for (auto &item : *data) {
//        qCDebug(BasicAlphaModelLog(), "data receveid %s", item.symbol.toStdString().c_str());
        tickers.append(item.symbol);
    }

    m_receivedDataCounter = 0;
    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_MONTH, "1 Y", "");
    for (const QString& ticker : tickers) {
        //auto id = getNextValidId();
        //histConfiguration.id = id;
        histConfiguration.symbol = ticker.toStdString().c_str();
        reqestHistoricalData(histConfiguration);
    }
}

void CBasicAlphaModel::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCInfo(BasicAlphaModelLog(), "symbol: [%s] - length: [%lld]", _symbol.toStdString().c_str(), _histMap.length());

    m_historicalData.insert(_symbol, _histMap);

    if(m_pProcessingData->length() == m_historicalData.count())
    {
        QMap<QString, double> momentumMap;
        qCInfo(BasicAlphaModelLog(), "Done!");
        for (const auto &symbol : m_historicalData.keys()) {
            double momentum = calculateMomentum(m_historicalData.value(symbol)); // Assuming each CHistoricalData contains historical data for one stock
            qCInfo(BasicAlphaModelLog(), "%s momentm = %f", symbol.toStdString().c_str(), momentum);
            momentumMap.insert(symbol, momentum); // Assuming symbol()
            //     gives the stock symbol
        }
        // Sort the symbols based on momentum
        QList<QPair<QString, double>> sortedMomentum;
        for (const QString &symbol : momentumMap.keys()) {
            sortedMomentum.append(qMakePair(symbol, momentumMap.value(symbol)));
        }
        std::sort(sortedMomentum.begin(), sortedMomentum.end(), [](const QPair<QString, double> &a, const QPair<QString, double> &b) {
            return a.second > b.second; // Sort in descending order
        });

        // Select top n stocks
        int n = 3; // Set the value of n as required
        qCInfo(BasicAlphaModelLog(), "Top %d momentums", n);
        QList<QString> topStocks;
        for (int i = 0; i < n && i < sortedMomentum.size(); ++i) {
            topStocks.append(sortedMomentum[i].first);
            qCInfo(BasicAlphaModelLog(), "%s : %f", sortedMomentum[i].first.toStdString().c_str(), sortedMomentum[i].second);
        }

    }


    emit dataProcessed(createDataList());
}

double CBasicAlphaModel::calculateMomentum(const QList<CHistoricalData> &data)
{
    // // Example: Momentum = Recent closing price - Closing price from the first day in the list
    double recentClose = data.last().getClose(); // Assuming getClose() is a method in CHistoricalData
    double pastClose = data.first().getClose();
    return (recentClose - pastClose)/pastClose;
}

