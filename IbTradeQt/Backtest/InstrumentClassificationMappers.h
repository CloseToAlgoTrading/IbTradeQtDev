#ifndef BACKTEST_INSTRUMENTCLASSIFICATIONMAPPERS_H
#define BACKTEST_INSTRUMENTCLASSIFICATIONMAPPERS_H

#include "Backtest/InstrumentClassification.h"
#include <QString>

namespace Backtest {

/// Map Yahoo chart meta.instrumentType / quoteType strings to coarse AssetKind.
AssetKind assetKindFromYahooInstrumentType(const QString& instrumentType,
                                           const QString& quoteType = QString());

/// Map IB secType (Contract::secType) to coarse AssetKind.
AssetKind assetKindFromIbSecType(const QString& secType);

} // namespace Backtest

#endif
