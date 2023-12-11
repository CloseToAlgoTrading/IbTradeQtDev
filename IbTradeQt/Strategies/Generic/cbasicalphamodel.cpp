
#include "cbasicalphamodel.h"
//#include <QDateTime>
#include <QStringList>

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

#define P_M_PERIOD_STR "Momentum period"
#define P_M_PERIOD_RES_STR "Momentum period resolution"
#define P_M_N "Number of elements"
#define I_ASSETS "Selected asssets"

#define TOP_N (this->m_ParametersMap[P_M_N].toInt())


CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_historicalData()
    , m_pProcessingData()
    , m_errorReceivedCounter(0)
{
    m_Name = "Base Alpha Model";
    this->setName("Base Alpha Model");

    this->m_ParametersMap.clear();
    this->m_ParametersMap[P_M_PERIOD_STR] = 1;
    this->m_ParametersMap[P_M_PERIOD_RES_STR] = "year";
    this->m_ParametersMap[P_M_N] = 3;

    this->m_genericInfo.clear();
    this->m_genericInfo[I_ASSETS] = "";

    m_pProcessedData = createDataList();

    QObject::connect(this, &CProcessingBase_v2::signalCbkRecvHistoricalData, this, &CBasicAlphaModel::slotCbkRecvHistoricalData, Qt::QueuedConnection);
    QObject::connect(this, &CProcessingBase_v2::signalErrorNotFound, this, &CBasicAlphaModel::slotErrorProcessing, Qt::QueuedConnection);
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

    m_errorReceivedCounter = 0;
    m_historicalData.clear();
    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_MONTH, "1 Y", "");
    for (const QString& ticker : tickers) {
        histConfiguration.symbol = ticker.toStdString().c_str();
        reqestHistoricalData(histConfiguration);
        qCDebug(BasicAlphaModelLog(), "requested id %d", histConfiguration.id);
    }
}

void CBasicAlphaModel::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCInfo(BasicAlphaModelLog(), "symbol: [%s] - length: [%lld]", _symbol.toStdString().c_str(), _histMap.length());

    m_historicalData.insert(_symbol, _histMap);

    if(m_pProcessingData->length() == m_historicalData.count()+m_errorReceivedCounter)
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
        int n = TOP_N; // Set the value of n as required
        qCInfo(BasicAlphaModelLog(), "Top %d momentums", n);
        QList<QString> topStocks;
        m_pProcessedData->clear();
        QString _assets;
        for (int i = 0; i < n && i < sortedMomentum.size(); ++i) {
            topStocks.append(sortedMomentum[i].first);
            qCInfo(BasicAlphaModelLog(), "%s : %f", sortedMomentum[i].first.toStdString().c_str(), sortedMomentum[i].second);

            m_pProcessedData->append(UnifiedModelData(sortedMomentum[i].first, DIRECTION_UP, 0.0, 0.0, m_historicalData.value(sortedMomentum[i].first).constLast().getClose()));

            _assets.append(sortedMomentum[i].first + ", ");
        }
        this->m_genericInfo[I_ASSETS] = _assets;

        emit dataProcessed(m_pProcessedData);
    }

}

void CBasicAlphaModel::slotErrorProcessing(int id)
{
    m_errorReceivedCounter++;
}

double CBasicAlphaModel::calculateMomentum(const QList<CHistoricalData> &data)
{
    // // Example: Momentum = Recent closing price - Closing price from the first day in the list
    double recentClose = data.last().getClose(); // Assuming getClose() is a method in CHistoricalData
    double pastClose = data.first().getClose();
    return (recentClose - pastClose)/pastClose;
}

