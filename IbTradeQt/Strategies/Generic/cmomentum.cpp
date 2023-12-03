
#include "cmomentum.h"
#include "UnifiedModelData.h"

#include <QSharedPointer>

Q_LOGGING_CATEGORY(MomentumPmLog, "Momentum.PM");

cMomentum::cMomentum(QObject *parent)
    : CBasicStrategy_V2{parent}
{
    m_Name = "Momentum";
    this->setName("Momentum");
}

bool cMomentum::start()
{
    if(!isConnectedTotheServer()) return false;
    CBasicStrategy_V2::start();
    auto m_pAssetList = createDataList();
    emit dataProcessed(m_pAssetList);
    return true;
}

bool cMomentum::stop()
{
    return CBasicStrategy_V2::stop();
}


