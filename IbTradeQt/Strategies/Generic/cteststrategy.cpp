#include "cteststrategy.h"

CTestStrategy::CTestStrategy(QObject *parent) : CBasicStrategy_V2(parent)
{
    this->m_InfoMap["name"] = "Strategy";
    this->m_ParametersMap["param_1"] = 10;
    this->m_ParametersMap["Param_2"] = "Test";
}
