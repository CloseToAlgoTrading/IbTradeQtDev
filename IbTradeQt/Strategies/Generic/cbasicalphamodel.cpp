
#include "cbasicalphamodel.h"
//#include <QDateTime>
#include <QStringList>

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

    QStringList tickers;
    for (auto &item : *data) {
//        qCDebug(BasicAlphaModelLog(), "data receveid %s", item.symbol.toStdString().c_str());
        tickers.append(item.symbol);
    }

    reqHistConfigData_t histConfiguration(0, BAR_SIZE_1_DAY, "1 Y", "");
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
    emit dataProcessed(createDataList());
}

