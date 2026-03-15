#ifndef BACKTEST_BACKTESTCONFIG_H
#define BACKTEST_BACKTESTCONFIG_H

#include <QString>
#include <QStringList>
#include <QDateTime>

namespace Backtest {

enum class BarResolution {
    Tick,    // real individual trades — JSONL and IB historical ticks only
    Sec5,    // 5-second bars — IB, PostgreSQL (if recorded at this resolution)
    Min1,
    Min5,
    Min15,
    Min30,
    Hour1,
    Day1     // Yahoo Finance maximum resolution
};

enum class FillModelType {
    Instant,     // fills at mid-price, same tick
    MidPrice,    // fills at mid-price
    BidAsk,      // buys at ask, sells at bid
    SlippageBps  // BidAsk + configurable slippage in basis points
};

enum class FillTiming {
    SignalOnClose_FillNextBarOpen,  // safe default — no look-ahead bias
    SignalOnTick_FillAtBidAsk,      // intrabar, fills at current bid/ask
    SignalOnClose_FillAtClose       // fills at close price — use only if intentional
};

struct BacktestConfig {
    QString         strategyConfigPath;
    QDateTime       startDate;
    QDateTime       endDate;
    QStringList     symbols;
    BarResolution   resolution    = BarResolution::Day1;
    double          initialCapital = 100'000.0;
    QString         dataSourceId;   // "postgres" | "csv" | "ib" | "yahoo" | "jsonl"
    QString         dataPath;       // file path or DB connection string (source-dependent)
    FillModelType   fillModel      = FillModelType::BidAsk;
    FillTiming      fillTiming     = FillTiming::SignalOnClose_FillNextBarOpen;
    double          slippageBps    = 1.0;

    // Optional benchmark symbol (e.g. "SPY"). When set, BacktestSession fetches
    // buy-and-hold data for this symbol and populates BacktestResult::benchmark.
    QString         benchmarkSymbol;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTCONFIG_H
