/* Copyright (C) 2023 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_COMMONDEFS_H
#define TWS_API_CLIENT_COMMONDEFS_H

typedef long TickerId;
typedef long OrderId;

enum faDataType { GROUPS=1, ALIASES=3 } ;

inline const char* faDataTypeStr ( faDataType pFaDataType )
{
	switch (pFaDataType) {
		case GROUPS:
			return "GROUPS";
		case ALIASES:
			return "ALIASES";
	}
	return 0 ;
}

enum MarketDataType { 
	REALTIME = 1, 
	FROZEN = 2,
	DELAYED = 3,
	DELAYED_FROZEN = 4
};

const std::string INFINITY_STR = "Infinity";

// FundAssetType
enum class FundAssetType {
    None,
    Others,
    MoneyMarket,
    FixedIncome,
    MultiAsset,
    Equity,
    Sector,
    Guaranteed,
    Alternative
};

// FundDistributionPolicyIndicator
enum class FundDistributionPolicyIndicator {
    None,
    AccumulationFund,
    IncomeFund
};
#endif /* common_defs_h_INCLUDED */
