#include "cbasicportfolio.h"

CBasicPortfolio::CBasicPortfolio(QObject *parent) : CBasicStrategy_V2(parent)
{
    this->m_InfoMap["name"] = "Portfolio";
    this->m_ParametersMap["Portfolio_Param"] = 100.1;
}


