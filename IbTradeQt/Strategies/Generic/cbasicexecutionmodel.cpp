
#include "cbasicexecutionmodel.h"
#include "dbdatatypes.h"
#include <QDateTime>
#include <QString>


Q_LOGGING_CATEGORY(BasicExecutionModelLog, "BasicExecutionModel.PM");

CBasicExecutionModel::CBasicExecutionModel(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_dbManager(parent)
    , m_Oder()
{
    m_Name = "Base Execution Model";
    this->setName("Base Execution Model");

    QObject::connect(this, &CProcessingBase_v2::signalRecvExecutionReport, this, &CBasicExecutionModel::slotRecvExecutionReport, Qt::AutoConnection);
}

bool CBasicExecutionModel::start()
{
    reqestOrderStatusSubscription();
    return true;
}

bool CBasicExecutionModel::stop()
{
    cancelOrderStatusSubscription();
    return false;
}

void CBasicExecutionModel::processData(DataListPtr data)
{
    static int tmp = 0;
    qCDebug(BasicExecutionModelLog(), "data receveid %lld", data->length());

    // for (const auto & item : *data) {
    //     qCDebug(BasicExecutionModelLog(), "%s : %d, %f", item.symbol.toStdString().c_str(), item.direction, item.amount);
    //     if(item.direction == DIRECTION_UP)
    //     {
    //         requestPlaceMarketOrder(item.symbol, item.amount, OA_BUY);
    //     }
    //     else if(item.direction == DIRECTION_DOWN)
    //     {
    //         requestPlaceMarketOrder(item.symbol, item.amount, OA_SELL);
    //     }
    // }

    m_Oder.setRemaining(1500);
    m_Oder.setDirection(OA_BUY);

    auto id = requestPlaceMarketOrder("BMW", 1500, OA_BUY);

    m_Oder.setId(id);


}

void CBasicExecutionModel::slotRecvExecutionReport(const CExecutionReport &obj)
{
    OpenPosition position;

    QDateTime currentDateTime = QDateTime::currentDateTime();

    m_Oder.setExecId(obj.getExecId());
    m_Oder.setFilled(obj.getAmount());

    if(0 >= m_Oder.getRemaining())
    {
        qCDebug(BasicExecutionModelLog(), "Done!");
    }
    qCDebug(BasicExecutionModelLog(), "%f/%f", m_Oder.getFilled(), m_Oder.getRemaining());


    DbTrade newTrade;
    newTrade.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
    newTrade.strategyId = 1;  // Example data
    newTrade.symbol = obj.getTicker();
    newTrade.quantity = obj.getAmount();
    newTrade.price = obj.getAvgPrice();
    newTrade.execId = obj.getExecId();
    // newTrade.pnl = 20.0;
    // newTrade.fee = 0.5;
    newTrade.tradeType = "BUY";

    m_dbManager.signalAddNewTrade(newTrade);
}


