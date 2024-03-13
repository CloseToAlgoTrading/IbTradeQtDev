
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
    QObject::connect(this, &CProcessingBase_v2::signalRecvCommissionReport, this, &CBasicExecutionModel::slotRecvCommissionReport, Qt::AutoConnection);
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

    m_Oder.setRemaining(336);
    m_Oder.setDirection(OA_SELL);

    auto id = requestPlaceMarketOrder("BMW", m_Oder.getRemaining(), m_Oder.getDirection());

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
    newTrade.strategyId = this->getId().toString(QUuid::WithoutBraces);  // Example data
    newTrade.symbol = obj.getTicker();
    newTrade.quantity = obj.getAmount();
    newTrade.price = obj.getAvgPrice();
    newTrade.execId = obj.getExecId();
    // newTrade.pnl = 20.0;
    // newTrade.fee = 0.5;
    newTrade.tradeType = m_Oder.getDirection() == OA_SELL ? "SELL" : "BUY";

    emit m_dbManager.signalAddNewTrade(newTrade);
}

void CBasicExecutionModel::slotRecvCommissionReport(const CCommissionReport &obj)
{
    if(!obj.getId().isEmpty())
    {
        DbTradeCommission DBComm;
        DBComm.execId = obj.getId();
        DBComm.fee = obj.getCommission();
        if(10000000000 > obj.getRealizedPNL())
        {
            DBComm.pnl = obj.getRealizedPNL();
        }
        emit m_dbManager.signalUpdateTradeCommision(DBComm);
    }
}


