/* Copyright (C) 2023 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"
#include "Utils.h"

bool Utils::isPegBenchOrder(std::string orderType) {
    return orderType == "PEG BENCH" || orderType == "PEGBENCH";
}

bool Utils::isPegMidOrder(std::string orderType) {
    return orderType == "PEG MID" || orderType == "PEGMID";
}

bool Utils::isPegBestOrder(std::string orderType) {
    return orderType == "PEG BEST" || orderType == "PEGBEST";
}

FundAssetType Utils::getFundAssetType(std::string value) {
    if (value == "000") {
        return FundAssetType::Others;
    }
    else if (value == "001") {
        return FundAssetType::MoneyMarket;
    }
    else if (value == "002") {
        return FundAssetType::FixedIncome;
    }
    else if (value == "003") {
        return FundAssetType::MultiAsset;
    }
    else if (value == "004") {
        return FundAssetType::Equity;
    }
    else if (value == "005") {
        return FundAssetType::Sector;
    }
    else if (value == "006") {
        return FundAssetType::Guaranteed;
    }
    else if (value == "007") {
        return FundAssetType::Alternative;
    }
    return FundAssetType::None;
}

FundDistributionPolicyIndicator Utils::getFundDistributionPolicyIndicator(std::string value) {
    if (value == "N") {
        return FundDistributionPolicyIndicator::AccumulationFund;
    }
    else if (value == "Y") {
        return FundDistributionPolicyIndicator::IncomeFund;
    }
    return FundDistributionPolicyIndicator::None;
}
