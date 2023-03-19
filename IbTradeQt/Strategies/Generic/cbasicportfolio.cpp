#include "cbasicportfolio.h"

CBasicPortfolio::CBasicPortfolio() : CBasicStrategy()
{
    this->m_InfoMap["name"] = "Portfolio";
    this->m_ParametersMap["Portfolio_Param"] = 100.1;
}


