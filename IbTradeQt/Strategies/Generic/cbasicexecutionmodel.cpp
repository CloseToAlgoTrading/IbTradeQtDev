
#include "cbasicexecutionmodel.h"
#include "dbdatatypes.h"
#include <QDateTime>
#include <QString>


Q_LOGGING_CATEGORY(BasicExecutionModelLog, "BasicExecutionModel.PM");

CBasicExecutionModel::CBasicExecutionModel(QObject *parent)
    : CBasicStrategy_V2{parent}
    , m_dbManager(parent)
{
    m_Name = "Base Execution Model";
    this->setName("Base Execution Model");

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

    requestPlaceMarketOrder("NVDA", 6, OA_SELL);

    OpenPosition position;

    QDateTime currentDateTime = QDateTime::currentDateTime();

    if(tmp == 0)
    {
        position.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
        position.fee = 0.5;
        position.price = 100.0;
        position.quantity = 100;
        position.status = PS_INIT_OPEN;
        position.strategyId = 10;
        position.symbol = "MVDA";

        m_dbManager.addCurrentPositionsState(position);
    }
    if(tmp == 1)
    {
        position.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
        position.fee = 0.5;
        position.price = 101.0;
        position.quantity = 100;
        position.status = PS_PARTIALY_CLOSED;
        position.strategyId = 10;
        position.symbol = "MVDA";
        m_dbManager.addCurrentPositionsState(position);

        position.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
        position.fee = 0.5;
        position.price = 100.0;
        position.quantity = 50;
        position.status = PS_OPEN;
        position.strategyId = 10;
        position.symbol = "MVDA";
        m_dbManager.addCurrentPositionsState(position);
    }
    if(tmp == 2)
    {
        position.date = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");
        position.fee = 0.5;
        position.price = 102.0;
        position.quantity = 50;
        position.status = PS_CLOSED;
        position.strategyId = 10;
        position.symbol = "MVDA";
        m_dbManager.addCurrentPositionsState(position);
    }
    tmp++;
}

