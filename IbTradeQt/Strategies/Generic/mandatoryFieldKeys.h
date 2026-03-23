#ifndef MANDATORYFIELDKEYS_H
#define MANDATORYFIELDKEYS_H

// ============================================================
// PARAMETERS -- user-editable configuration
// ============================================================
namespace MandatoryParams {

    // Shared across Account, Portfolio, Strategy (inheritable)
    constexpr auto Name         = "Name";
    constexpr auto Description  = "Description";
    constexpr auto BenchmarkRef = "BenchmarkRef";

    // Account-only
    namespace Account {
        constexpr auto BrokerType           = "BrokerType";
        constexpr auto ConnectionProfileRef = "ConnectionProfileRef";
        constexpr auto AccountId            = "AccountId";
    }

    // Strategy-only
    namespace Strategy {
        constexpr auto Priority = "Priority";
    }

    // Portfolio has no unique params
}

// ============================================================
// INFO -- read-only runtime data
// ============================================================
namespace MandatoryInfo {
    namespace Account {
        constexpr auto BrokerName          = "BrokerName";
        constexpr auto AccountType         = "AccountType";
        constexpr auto BaseCurrency        = "BaseCurrency";
        constexpr auto Status              = "Status";
        constexpr auto LastSyncTime        = "LastSyncTime";
        constexpr auto NetLiquidation      = "NetLiquidation";
        constexpr auto CashBalance         = "CashBalance";
        constexpr auto BuyingPower         = "BuyingPower";
        constexpr auto EquityWithLoanValue = "EquityWithLoanValue";
        constexpr auto AvailableFunds      = "AvailableFunds";
        constexpr auto MaintMarginReq      = "MaintMarginReq";
        constexpr auto InitialMarginReq    = "InitialMarginReq";
        constexpr auto DayTradesRemaining  = "DayTradesRemaining";
        constexpr auto BrokerPermissions   = "BrokerPermissions";
    }
    namespace Portfolio {
        constexpr auto Status             = "Status";
        constexpr auto LastValuationTime  = "LastValuationTime";
        constexpr auto MarketValue        = "MarketValue";
        constexpr auto UnrealizedPnL      = "UnrealizedPnL";
        constexpr auto RealizedPnL        = "RealizedPnL";
        constexpr auto DailyPnL           = "DailyPnL";
        constexpr auto TotalReturnPct     = "TotalReturnPct";
        constexpr auto ExposureGross      = "ExposureGross";
        constexpr auto ExposureNet        = "ExposureNet";
        constexpr auto PositionsCount     = "PositionsCount";
        constexpr auto BenchmarkValue     = "BenchmarkValue";
        constexpr auto BenchmarkReturnPct = "BenchmarkReturnPct";
    }
    namespace Strategy {
        constexpr auto Status             = "Status";
        constexpr auto LastRunTime        = "LastRunTime";
        constexpr auto ActiveModels       = "ActiveModels";
        constexpr auto PositionsCount     = "PositionsCount";
        constexpr auto OpenOrdersCount    = "OpenOrdersCount";
        constexpr auto UnrealizedPnL      = "UnrealizedPnL";
        constexpr auto RealizedPnL        = "RealizedPnL";
        constexpr auto DailyPnL           = "DailyPnL";
        constexpr auto DrawdownPct        = "DrawdownPct";
        constexpr auto BenchmarkValue     = "BenchmarkValue";
        constexpr auto BenchmarkReturnPct = "BenchmarkReturnPct";
        constexpr auto LastSignalTime     = "LastSignalTime";
        constexpr auto LastRebalanceTime  = "LastRebalanceTime";
    }
}

// ============================================================
// ASSET FIELDS -- per-position property schemas
// ============================================================
namespace AssetFields {

    // Strategy and Portfolio positions (shared schema)
    namespace Position {
        constexpr auto PnL      = "PnL";
        constexpr auto AvgPrice = "AvgPrice";
        constexpr auto Quantity = "Quantity";
        /// Strategy-scoped coarse AssetKind override (string, e.g. "Etf"); empty = auto from resolver
        constexpr auto ClassificationOverride = "classificationOverride";
        /// Optional session/calendar policy id or label for strategy logic (not AssetKind)
        constexpr auto SessionPolicy          = "sessionPolicy";
    }

    // Account broker positions (richer schema)
    namespace BrokerPosition {
        constexpr auto Quantity      = "Quantity";
        constexpr auto AvgCost       = "AvgCost";
        constexpr auto MarketPrice   = "MarketPrice";
        constexpr auto MarketValue   = "MarketValue";
        constexpr auto UnrealizedPnL = "UnrealizedPnL";
        constexpr auto SecurityType  = "SecurityType";
        constexpr auto Currency      = "Currency";
        constexpr auto Exchange      = "Exchange";
    }
}

#endif // MANDATORYFIELDKEYS_H
