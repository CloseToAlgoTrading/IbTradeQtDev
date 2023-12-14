
#include "cbasicexecutionmodel.h"

Q_LOGGING_CATEGORY(BasicExecutionModelLog, "BasicExecutionModel.PM");

CBasicExecutionModel::CBasicExecutionModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Execution Model";
    this->setName("Base Execution Model");
}

void CBasicExecutionModel::processData(DataListPtr data)
{
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

}

