#include "PaperBrokerStub.h"

namespace Brokers {

PaperBrokerStub::PaperBrokerStub(QObject* parent)
    : IBrokerAPI()
{
    setParent(parent);
}

void PaperBrokerStub::processMessagesAPI() {}

bool PaperBrokerStub::connectAPI(const char*, unsigned int, int)
{
    return false;
}

bool PaperBrokerStub::isConnectedAPI()
{
    return false;
}

void PaperBrokerStub::disconnectAPI() {}

void PaperBrokerStub::reqCurrentTimeAPI() {}

void PaperBrokerStub::reqRealTimeDataAPI(qint32 _id, reqReadlTimeDataConfigData_t& _config)
{
    Q_UNUSED(_id)
    Q_UNUSED(_config)
}

void PaperBrokerStub::cancelRealTimeDataAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::reqHistoricalDataAPI(const reqHistConfigData_t& _config)
{
    Q_UNUSED(_config)
}

void PaperBrokerStub::cancelHistoricalDataAPI(qint32 id)
{
    Q_UNUSED(id)
}

void PaperBrokerStub::reqRealTimeBarsAPI(qint32 _id, const QString& _symbol)
{
    Q_UNUSED(_id)
    Q_UNUSED(_symbol)
}

void PaperBrokerStub::cancelRealTimeBarsAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::reqPositionAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::cancelPositionAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::reqCalculateOptionPriceAPI(const reqCalcOptPriceConfigData_t& _config)
{
    Q_UNUSED(_config)
}

void PaperBrokerStub::cancelCalculateOptionPriceAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::reqHistoricalTicksAPI(const reqHistTicksConfigData_t& _config)
{
    Q_UNUSED(_config)
}

void PaperBrokerStub::reqTickByTickDataAPI(const reqTickByTickDataConfigData_t& _config)
{
    Q_UNUSED(_config)
}

void PaperBrokerStub::cancelTickByTickDataAPI(qint32 id)
{
    Q_UNUSED(id)
}

void PaperBrokerStub::reqAccountSummary() {}

void PaperBrokerStub::cancelAccountSummary(qint32 id)
{
    Q_UNUSED(id)
}

qint32 PaperBrokerStub::reqPlaceOrderAPI(const QString&, qint32, eOrderAction_t)
{
    return -1;
}

void PaperBrokerStub::cancelOrderAPI(qint32 _id)
{
    Q_UNUSED(_id)
}

void PaperBrokerStub::reqOpenOrdersAPI() {}

void PaperBrokerStub::reqAllOpenOrdersAPI() {}

void PaperBrokerStub::reqAutoOpenOrdersAPI(bool _bAutoBind)
{
    Q_UNUSED(_bAutoBind)
}

void PaperBrokerStub::reqNextValidIDsAPI(qint32 _numIds)
{
    Q_UNUSED(_numIds)
}

void PaperBrokerStub::reqGlobalCancelAPI() {}

} // namespace Brokers
