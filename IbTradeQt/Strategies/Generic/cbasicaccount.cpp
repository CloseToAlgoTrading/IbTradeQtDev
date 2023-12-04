#include "cbasicaccount.h"

CBasicAccount::CBasicAccount(QObject *parent) : CBasicStrategy_V2(parent)
{
    this->m_InfoMap["IsParentActivated"] = true;

    m_Name = "Account";
    this->m_InfoMap["name"] = "Account";
    this->m_ParametersMap["Account_Param"] = 11.1;
    this->m_ParametersMap["Account_Param2"] = 2;
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
    }
    else if((false == state) && (true == getActiveStatus()) && (true == getParentActivatedState()))
    {
        stop();
    }
}
