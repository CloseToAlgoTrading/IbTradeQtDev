
#include "cbasicalphamodel.h"
//#include <QDateTime>

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Alpha Model";
    this->setName("Base Alpha Model");

    QObject::connect(this, &CProcessingBase_v2::signalCbkRecvHistoricalData, this, &CBasicAlphaModel::slotCbkRecvHistoricalData, Qt::QueuedConnection);
}

void CBasicAlphaModel::processData(DataListPtr data)
{
    auto item = data->constLast();
    qCDebug(BasicAlphaModelLog(), "data receveid %lld %s %f", data->length(), item.symbol.toStdString().c_str(), item.amount);

        QList<QString> tickers = {"AAPL", "MSFT", "GOOG"};
        //QList<QString> tickers = {"AAPL"};

        //QString queryTime = QDateTime::currentDateTime().toUTC().toString("yyyyMMdd-HH:mm:ss");

        reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "5 D", "");
        for (const QString& ticker : tickers) {
            //auto id = getNextValidId();
            //histConfiguration.id = id;
            histConfiguration.symbol = ticker.toStdString().c_str();
            //reqestHistoricalData(histConfiguration);
        }
    emit dataProcessed(createDataList());
}

void CBasicAlphaModel::slotCbkRecvHistoricalData(const QList<CHistoricalData> &_histMap, const QString &_symbol)
{
    qCDebug(BasicAlphaModelLog(), "symbol: [%s] - length: [%lld]", _symbol.toStdString().c_str(), _histMap.length());
    emit dataProcessed(createDataList());
}

