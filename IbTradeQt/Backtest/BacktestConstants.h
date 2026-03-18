#pragma once
#include <QLatin1StringView>

// ─────────────────────────────────────────────────────────────────────────────
// Single source of truth for backtest-related string constants and numeric
// magic values.
// ─────────────────────────────────────────────────────────────────────────────

namespace Backtest {

namespace Status {
    inline constexpr QLatin1StringView Created   { "Created"   };
    inline constexpr QLatin1StringView Running   { "Running"   };
    inline constexpr QLatin1StringView Finished  { "Finished"  };
    inline constexpr QLatin1StringView Failed    { "Failed"    };
    inline constexpr QLatin1StringView Cancelled { "Cancelled" };
} // namespace Status

namespace Scope {
    inline constexpr QLatin1StringView Strategy  { "strategy"  };
    inline constexpr QLatin1StringView Portfolio { "portfolio" };
    inline constexpr QLatin1StringView Account   { "account"   };
} // namespace Scope

// Maximum milliseconds to wait for the worker thread to finish during a
// graceful shutdown.  Callers that cannot tolerate this latency should cancel
// the session before destruction.
inline constexpr int kThreadShutdownTimeoutMs = 5000;

} // namespace Backtest
