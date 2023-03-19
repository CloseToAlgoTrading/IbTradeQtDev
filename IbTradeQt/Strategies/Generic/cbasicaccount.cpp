#include "cbasicaccount.h"

CBasicAccount::CBasicAccount() : CBasicStrategy()
{
    this->m_InfoMap["name"] = "Account";
    this->m_ParametersMap["Account_Param"] = 11.1;
}
