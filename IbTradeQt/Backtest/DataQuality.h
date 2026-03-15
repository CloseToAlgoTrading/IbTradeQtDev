#ifndef BACKTEST_DATAQUALITY_H
#define BACKTEST_DATAQUALITY_H

namespace Backtest {

// Describes the fidelity of the data used in a backtest run.
// Shown prominently in the result UI so users cannot miss the caveat.
enum class DataQuality {
    RealTicks,         // tick-level source (JSONL recorded session, IB historical ticks)
    SynthesizedOHLC,   // bar-level source with 4-tick OHLC synthesis — indicative only
    DailyBars          // daily bar source (Yahoo Finance) — coarsest approximation
};

} // namespace Backtest

#endif // BACKTEST_DATAQUALITY_H
