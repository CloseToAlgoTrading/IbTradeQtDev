/* Copyright (C) 2023 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */
#pragma once
#ifndef TWS_API_CLIENT_UTILS_H
#define TWS_API_CLIENT_UTILS_H

#include <string>
#include "CommonDefs.h"

class Utils {

public:
    
    static bool isPegBenchOrder(std::string orderType);
    static bool isPegMidOrder(std::string orderType);
    static bool isPegBestOrder(std::string orderType);
    static FundDistributionPolicyIndicator getFundDistributionPolicyIndicator(std::string value);
    static FundAssetType getFundAssetType(std::string value);
};

#endif

