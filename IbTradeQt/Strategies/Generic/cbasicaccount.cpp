#include "cbasicaccount.h"

CBasicAccount::CBasicAccount(QObject *parent) : CBasicStrategy_V2(parent)
{
    this->m_InfoMap["name"] = "Account";
    this->m_ParametersMap["Account_Param"] = 11.1;
    this->m_ParametersMap["Account_Param2"] = 2;
}
