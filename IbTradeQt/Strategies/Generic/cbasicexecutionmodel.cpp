
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

}

