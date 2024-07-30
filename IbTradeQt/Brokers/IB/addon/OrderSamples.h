/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_SAMPLES_TESTCPPCLIENT_ORDERSAMPLES_H
#define TWS_API_SAMPLES_TESTCPPCLIENT_ORDERSAMPLES_H

#include <vector>

struct Order;
class OrderCondition;

class OrderSamples {
public:
	static Order AtAuction(std::string action, Decimal quantity, double price);
	static Order Discretionary(std::string action, Decimal quantity, double price, double discretionaryAmount);
	static Order MarketOrder(std::string action, Decimal quantity);
	static Order MarketIfTouched(std::string action, Decimal quantity, double price);
	static Order MarketOnClose(std::string action, Decimal quantity);
	static Order MarketOnOpen(std::string action, Decimal quantity);
	static Order MidpointMatch(std::string action, Decimal quantity);
	static Order Midprice(std::string action, Decimal quantity, double priceCap);
	static Order PeggedToMarket(std::string action, Decimal quantity, double marketOffset);
	static Order PeggedToStock(std::string action, Decimal quantity, double delta, double stockReferencePrice, double startingPrice);
	static Order RelativePeggedToPrimary(std::string action, Decimal quantity, double priceCap, double offsetAmount);
	static Order SweepToFill(std::string action, Decimal quantity, double price);
	static Order AuctionLimit(std::string action, Decimal quantity, double price, int auctionStrategy);
	static Order AuctionPeggedToStock(std::string action, Decimal quantity, double startingPrice, double delta);
	static Order AuctionRelative(std::string action, Decimal quantity, double offset);
	static Order Block(std::string action, Decimal quantity, double price);
	static Order BoxTop(std::string action, Decimal quantity);
	static Order LimitOrder(std::string action, Decimal quantity, double limitPrice);
	static Order LimitOrderWithCashQty(std::string action, double limitPrice, double cashQty);
	static Order LimitIfTouched(std::string action, Decimal quantity, double limitPrice, double triggerPrice);
	static Order LimitOnClose(std::string action, Decimal quantity, double limitPrice);
	static Order LimitOnOpen(std::string action, Decimal quantity, double limitPrice);
	static Order PassiveRelative(std::string action, Decimal quantity, double offset);
	static Order PeggedToMidpoint(std::string action, Decimal quantity, double offset, double limitPrice);
	static void BracketOrder(int parentOrderId, Order& parent, Order& takeProfit, Order& stopLoss, std::string action, Decimal quantity, double limitPrice, double takeProfitLimitPrice, double stopLossPrice);
	static Order MarketToLimit(std::string action, Decimal quantity);
	static Order MarketWithProtection(std::string action, Decimal quantity);
	static Order Stop(std::string action, Decimal quantity, double stopPrice);
	static Order StopLimit(std::string action, Decimal quantity, double limitPrice, double stopPrice);
	static Order StopWithProtection(std::string action, Decimal quantity, double stopPrice);
	static Order TrailingStop(std::string action, Decimal quantity, double trailingPercent, double trailStopPrice);
	static Order TrailingStopLimit(std::string action, Decimal quantity, double lmtPriceOffset, double trailingAmount, double trailStopPrice);
	static Order ComboLimitOrder(std::string action, Decimal quantity, double limitPrice, bool nonGuaranteed);
	static Order ComboMarketOrder(std::string action, Decimal quantity, bool nonGuaranteed);
	static Order LimitOrderForComboWithLegPrices(std::string action, Decimal quantity, std::vector<double> legprices, bool nonGuaranteed);
	static Order RelativeLimitOrder(std::string action, Decimal quantity, double limitPrice, bool nonGuaranteed);
	static Order RelativeMarketCombo(std::string action, Decimal quantity, bool nonGuaranteed);
	static void OneCancelsAll(std::string ocaGroup, Order& ocaOrder, int ocaType);
	static Order Volatility(std::string action, Decimal quantity, double volatilityPercent, int volatilityType);
	static Order MarketFHedge(int parentOrderId, std::string action);
	static Order PeggedToBenchmark(std::string action, Decimal quantity, double startingPrice, bool peggedChangeAmountDecrease, double peggedChangeAmount, double referenceChangeAmount, int referenceConId, std::string referenceExchange, double stockReferencePrice, double referenceContractLowerRange, double referenceContractUpperRange);
	static Order AttachAdjustableToStop(Order parent, double attachedOrderStopPrice, double triggerPrice, double adjustStopPrice);
	static Order AttachAdjustableToStopLimit(Order parent, double attachedOrderStopPrice, double triggerPrice, double adjustStopPrice, double adjustedStopLimitPrice);
	static Order AttachAdjustableToTrail(Order parent, double attachedOrderStopPrice, double triggerPrice, double adjustStopPrice, double adjustedTrailAmount, int trailUnit);
	static Order WhatIfLimitOrder(std::string action, Decimal quantity, double limitPrice);
	static OrderCondition* Price_Condition(int conId, std::string exchange, double price, bool isMore, bool isConjunction);
	static OrderCondition* Execution_Condition(std::string symbol, std::string secType, std::string exchange, bool isConjunction);
	static OrderCondition* Margin_Condition(int percent, bool isMore, bool isConjunction);
	static OrderCondition* Percent_Change_Condition(double pctChange, int conId, std::string exchange, bool isMore, bool isConjunction);
	static OrderCondition* Time_Condition(std::string time, bool isMore, bool isConjunction);
	static OrderCondition* Volume_Condition(int conId, std::string exchange, bool isMore, int volume, bool isConjunction);
	static Order LimitIBKRATS (std::string action, Decimal quantity, double limitPrice);
	static Order LimitOrderWithManualOrderTime(std::string action, Decimal quantity, double limitPrice, std::string manualOrderTime);
	static Order PegBestUpToMidOrder(std::string action, Decimal quantity, double limitPrice, int minTradeQty, int minCompeteSize, double midOffsetAtWhole, double midOffsetAtHalf);
	static Order PegBestOrder(std::string action, Decimal quantity, double limitPrice, int minTradeQty, int minCompeteSize, double competeAgainstBestOffset);
	static Order PegMidOrder(std::string action, Decimal quantity, double limitPrice, int minTradeQty, double midOffsetAtWhole, double midOffsetAtHalf);
};

#endif
