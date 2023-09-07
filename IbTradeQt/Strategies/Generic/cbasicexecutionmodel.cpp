
#include "cbasicexecutionmodel.h"

Q_LOGGING_CATEGORY(BasicExecutionModelLog, "BasicExecutionModel.PM");

CBasicExecutionModel::CBasicExecutionModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    this->setName("Base Execution Model");
}

