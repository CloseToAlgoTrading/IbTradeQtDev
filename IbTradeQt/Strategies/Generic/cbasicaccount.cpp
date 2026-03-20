#include "cbasicaccount.h"
#include "modelConstants.h"
#include "mandatoryFieldRegistration.h"
#include "mandatoryFieldKeys.h"

CBasicAccount::CBasicAccount(QObject *parent) : CBaseModel(parent)
{
    this->m_InfoMap[CIM_IsParentActivated] = true;
    MandatoryFieldRegistration::registerAccountFields(*this);

    QObject::connect(this, &CProcessingBase_v2::signalRecvAccountSummary, this, &CBasicAccount::slotRecvAccountSummary, Qt::AutoConnection);
    QObject::connect(this, &CProcessingBase_v2::signalEndRecvPosition, this, &CBasicAccount::slotEndRecvPosition, Qt::QueuedConnection);
}

void CBasicAccount::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient)
{
    // Disconnect from old provider before replacing it
    auto old = getIBrokerDataProvider();
    if (old && old->getClien()) {
        QObject::disconnect(old->getClien().data(), &IBrokerAPI::signalServerStateUpdate,
                            this, &CBaseModel::onUpdateServerConnectionStateSlot);
    }

    CBaseModel::setBrokerDataProvider(newClient);

    // Connect to new provider only when both are valid
    if (newClient && newClient->getClien()) {
        QObject::connect(newClient->getClien().data(), &IBrokerAPI::signalServerStateUpdate,
                         this, &CBaseModel::onUpdateServerConnectionStateSlot);
    }
}

/* Temporary here.. probably need to make a generic in base */
void CBasicAccount::onUpdateServerConnectionStateSlot(bool state)
{
    qCDebug(lcBaseModel) << "--> server state: " << ((state == true) ? "Connected" : "Disconnected");
    m_genericInfo[MandatoryInfo::Account::Status] = state ? "Connected" : "Disconnected";

    if((true == state) && (true == getActiveStatus()) && (true == getParentActivatedState()))
    {
        start();
        pbReqAccountSummary();
        pbRequestPosition();
    }
    else if((false == state) && (true == getActiveStatus()) && (true == getParentActivatedState()))
    {
        stop();
        //TODO: Error -> check if it working!
        pbCancelAccountSummary();
        pbCancelPosition();
    }
}

void CBasicAccount::slotRecvAccountSummary(const CAccountSummary &obj)
{
    m_genericInfo[MandatoryInfo::Account::AccountType]         = obj.getAccountType();
    m_genericInfo[MandatoryInfo::Account::BaseCurrency]        = obj.getCurrency();
    m_genericInfo[MandatoryInfo::Account::BuyingPower]         = obj.getBuyingPower();
    m_genericInfo[MandatoryInfo::Account::CashBalance]         = obj.getTotalCashValue();
    m_genericInfo[MandatoryInfo::Account::NetLiquidation]      = obj.getNetLiquidation();
    m_genericInfo[MandatoryInfo::Account::EquityWithLoanValue] = obj.getEquityWithLoanValue();
}

void CBasicAccount::slotEndRecvPosition()
{
    m_assetList.clear();
    for (auto iter = m_positionMap.constBegin(); iter != m_positionMap.constEnd(); ++iter)
    {
        if(iter.value().getPos() != 0.0)
        {
            this->m_assetList[iter.key()] = createAssetEntry({
                {AssetFields::BrokerPosition::Quantity,     iter.value().getPos()},
                {AssetFields::BrokerPosition::AvgCost,      iter.value().getAvgCost()},
                {AssetFields::BrokerPosition::SecurityType, iter.value().getContract().secType.c_str()},
                {AssetFields::BrokerPosition::Currency,     iter.value().getContract().currency.c_str()},
                {AssetFields::BrokerPosition::Exchange,     iter.value().getContract().exchange.c_str()}
            });
        }
    }
}

