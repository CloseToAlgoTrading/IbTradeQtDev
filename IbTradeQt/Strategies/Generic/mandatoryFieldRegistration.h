#ifndef MANDATORYFIELDREGISTRATION_H
#define MANDATORYFIELDREGISTRATION_H

#include "IMandatoryFields.h"
#include "mandatoryFieldKeys.h"

namespace MandatoryFieldRegistration {

inline void registerAccountFields(IMandatoryFields& m)
{
    // Params
    m.registerMandatoryParam(MandatoryParams::Name, "Account");
    m.registerMandatoryParam(MandatoryParams::Description, "");
    m.registerInheritableParam(MandatoryParams::BenchmarkRef, "");
    m.registerMandatoryParam(MandatoryParams::Account::BrokerType, "");
    m.registerMandatoryParam(MandatoryParams::Account::ConnectionProfileRef, "");
    m.registerMandatoryParam(MandatoryParams::Account::AccountId, "");

    // Info
    m.registerMandatoryInfo(MandatoryInfo::Account::BrokerName, "");
    m.registerMandatoryInfo(MandatoryInfo::Account::AccountType, "");
    m.registerMandatoryInfo(MandatoryInfo::Account::BaseCurrency, "");
    m.registerMandatoryInfo(MandatoryInfo::Account::Status, "Disconnected");
    m.registerMandatoryInfo(MandatoryInfo::Account::LastSyncTime, "");
    m.registerMandatoryInfo(MandatoryInfo::Account::NetLiquidation, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::CashBalance, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::BuyingPower, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::EquityWithLoanValue, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::AvailableFunds, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::MaintMarginReq, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::InitialMarginReq, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Account::DayTradesRemaining, "");
    m.registerMandatoryInfo(MandatoryInfo::Account::BrokerPermissions, "");

    // Asset fields
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::Quantity, 0.0);
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::AvgCost, 0.0);
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::MarketPrice, 0.0);
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::MarketValue, 0.0);
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::UnrealizedPnL, 0.0);
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::SecurityType, "");
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::Currency, "");
    m.registerMandatoryAssetField(AssetFields::BrokerPosition::Exchange, "");
}

inline void registerPortfolioFields(IMandatoryFields& m)
{
    // Params
    m.registerMandatoryParam(MandatoryParams::Name, "Portfolio");
    m.registerMandatoryParam(MandatoryParams::Description, "");
    m.registerInheritableParam(MandatoryParams::BenchmarkRef, "");

    // Info
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::Status, "Inactive");
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::LastValuationTime, "");
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::MarketValue, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::UnrealizedPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::RealizedPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::DailyPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::TotalReturnPct, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::ExposureGross, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::ExposureNet, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::PositionsCount, 0);
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::BenchmarkValue, "");
    m.registerMandatoryInfo(MandatoryInfo::Portfolio::BenchmarkReturnPct, 0.0);

    // Asset fields (same schema as strategy positions)
    m.registerMandatoryAssetField(AssetFields::Position::PnL, 0.0);
    m.registerMandatoryAssetField(AssetFields::Position::AvgPrice, 0.0);
    m.registerMandatoryAssetField(AssetFields::Position::Quantity, 0.0);
}

inline void registerStrategyFields(IMandatoryFields& m)
{
    // Params
    m.registerMandatoryParam(MandatoryParams::Name, "Strategy");
    m.registerMandatoryParam(MandatoryParams::Description, "");
    m.registerInheritableParam(MandatoryParams::BenchmarkRef, "");
    m.registerMandatoryParam(MandatoryParams::Strategy::Priority, 1);

    // Info
    m.registerMandatoryInfo(MandatoryInfo::Strategy::Status, "Stopped");
    m.registerMandatoryInfo(MandatoryInfo::Strategy::LastRunTime, "");
    m.registerMandatoryInfo(MandatoryInfo::Strategy::ActiveModels, 0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::PositionsCount, 0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::OpenOrdersCount, 0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::UnrealizedPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::RealizedPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::DailyPnL, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::DrawdownPct, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::BenchmarkValue, "");
    m.registerMandatoryInfo(MandatoryInfo::Strategy::BenchmarkReturnPct, 0.0);
    m.registerMandatoryInfo(MandatoryInfo::Strategy::LastSignalTime, "");
    m.registerMandatoryInfo(MandatoryInfo::Strategy::LastRebalanceTime, "");

    // Asset fields (shared Position schema)
    m.registerMandatoryAssetField(AssetFields::Position::PnL, 0.0);
    m.registerMandatoryAssetField(AssetFields::Position::AvgPrice, 0.0);
    m.registerMandatoryAssetField(AssetFields::Position::Quantity, 0.0);
}

} // namespace MandatoryFieldRegistration

#endif // MANDATORYFIELDREGISTRATION_H
