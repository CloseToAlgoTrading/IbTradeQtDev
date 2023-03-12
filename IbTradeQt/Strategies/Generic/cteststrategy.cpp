#include "cteststrategy.h"

CTestStrategy::CTestStrategy() : CBasicStrategy()
{
    this->m_InfoMap["name"] = "Strategy";
    this->m_ParametersMap["param_1"] = 10;
    this->m_ParametersMap["Param_2"] = "Test";
}
