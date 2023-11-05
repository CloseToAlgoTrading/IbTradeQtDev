#include "cbasicportfolio.h"

CBasicPortfolio::CBasicPortfolio(QObject *parent) : CBasicStrategy_V2(parent)
{
    m_Name = "Portfolio";
    this->m_InfoMap["name"] = "Portfolio";
    this->m_ParametersMap["Portfolio_Param"] = 100.1;
}


