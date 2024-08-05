#include "cbasicaccount.h"
#include "modelConstants.h"

CBasicAccount::CBasicAccount(QObject *parent) : CBaseModel(parent)
{
    this->m_InfoMap[CIM_IsParentActivated] = true;

    m_Name = "Account";
    this->m_InfoMap["name"] = "Account";
    this->m_ParametersMap["Account"] = "Enter Your Account ID";
    this->m_ParametersMap["Account_Param2"] = 2;

    QObject::connect(this, &CProcessingBase_v2::signalRecvAccountSummary, this, &CBasicAccount::slotRecvAccountSummary, Qt::AutoConnection);
    QObject::connect(this, &CProcessingBase_v2::signalEndRecvPosition, this, &CBasicAccount::slotEndRecvPosition, Qt::QueuedConnection);
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
    m_genericInfo["1. Account"] = obj.getAccount();
    m_genericInfo["2. Account Type"] = obj.getAccountType();
    m_genericInfo["3. Currency"] = obj.getCurrency();
    m_genericInfo["4. Buying Power"] = obj.getBuyingPower();
    m_genericInfo["5. Total Cash Value"] = obj.getTotalCashValue();
    m_genericInfo["6. Net Liquidation"] = obj.getNetLiquidation();
    m_genericInfo["7. Equity With Loan Value"] = obj.getEquityWithLoanValue();
}

void CBasicAccount::slotEndRecvPosition()
{
    m_assetList.clear();
    // find all stock position according to ticketName
    for (auto iter = m_positionMap.constBegin(); iter != m_positionMap.constEnd(); ++iter)
    {
        if(iter.value().getPos() != 0.0)
        {
            //qDebug("Ticker [%s] size[%f] cost[%f]", iter.key().toStdString().c_str(), iter.value().getPos(), iter.value().getAvgCost());
            this->m_assetList[iter.key()] = QVariantMap({
                                                         {"1. Size", iter.value().getPos()},
                                                         {"2. Avg. Cost",iter.value().getAvgCost()},
                                                         {"3. Type", iter.value().getContract().secType.c_str()},
                                                         {"4. Currency", iter.value().getContract().currency.c_str()},
                                                         {"5. Prim. Exchange", iter.value().getContract().exchange.c_str()}
            });
        }

    }
}

