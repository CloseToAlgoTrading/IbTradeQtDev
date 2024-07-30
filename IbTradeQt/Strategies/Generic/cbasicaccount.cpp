#include "cbasicaccount.h"
#include "modelConstants.h"

CBasicAccount::CBasicAccount(QObject *parent) : CBaseModel(parent)
{
    this->m_InfoMap[CIM_IsParentActivated] = true;

    m_Name = "Account";
    this->m_InfoMap["name"] = "Account";
    this->m_ParametersMap["Account_Param"] = 11.1;
    this->m_ParametersMap["Account_Param2"] = 2;

    QObject::connect(this, &CProcessingBase_v2::signalRecvAccountSummary, this, &CBasicAccount::signalRecvAccountSummary, Qt::AutoConnection);
}

void CBasicAccount::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient)
{
    CBaseModel::setBrokerDataProvider(newClient);
    QObject::disconnect(this->getIBrokerDataProvider()->getClien().data(), &IBrokerAPI::signalServerStateUpdate, this, &CBaseModel::onUpdateServerConnectionStateSlot);
    QObject::connect(this->getIBrokerDataProvider()->getClien().data(), &IBrokerAPI::signalServerStateUpdate, this, &CBaseModel::onUpdateServerConnectionStateSlot);
}

/* Temporary here.. probably need to make a generic in base */
void CBasicAccount::onUpdateServerConnectionStateSlot(bool state)
{
    qDebug() << "--> server state: " << ((state == true) ? "Connected" : "Disconnected");

    if((true == state) && (true == getActiveStatus()) && (true == getParentActivatedState()))
    {
        start();
        pbReqAccountSummary();
    }
    else if((false == state) && (true == getActiveStatus()) && (true == getParentActivatedState()))
    {
        stop();
        //TODO: Error -> check if it working!
        pbCancelAccountSummary();
    }
}

void CBasicAccount::signalRecvAccountSummary(const CAccountSummary &obj)
{
    m_genericInfo["Account"] = obj.getAccount();
    m_genericInfo["Account Type"] = obj.getAccountType();
    m_genericInfo["Currency"] = obj.getCurrency();
    m_genericInfo["Buying Power"] = obj.getBuyingPower();
    m_genericInfo["Total Cash Value"] = obj.getTotalCashValue();
    m_genericInfo["Net Liquidation"] = obj.getNetLiquidation();
    m_genericInfo["Equity With Loan Value"] = obj.getEquityWithLoanValue();
}

