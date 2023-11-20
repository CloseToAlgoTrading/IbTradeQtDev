
#include "cbasicalphamodel.h"

Q_LOGGING_CATEGORY(BasicAlphaModelLog, "BasicAlphaModel.PM");

CBasicAlphaModel::CBasicAlphaModel(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Base Alpha Model";
    this->setName("Base Alpha Model");
}

void CBasicAlphaModel::processData(DataListPtr data)
{
    auto item = data->constLast();
    qCDebug(BasicAlphaModelLog(), "data receveid %d %s %f", data->length(), item.symbol.toStdString().c_str(), item.amount);

}

