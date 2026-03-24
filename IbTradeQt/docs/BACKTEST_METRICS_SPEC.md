# Backtest metrics specification

**Authority:** Implemented formulas live in `BacktestStatisticsCalculator` (C++). Persisted `BacktestMetrics` SQLite rows are a cache; on formula version changes, **recompute** or treat stored values as stale per `metricDefinitionsVersion` in JSON exports.

**metricDefinitionsVersion:** `2` — adds average exposure, annualized turnover, monthly/yearly period-return arrays in JSON; `1` was the first documented scalar extended set.

---

## Conventions

| Topic | Rule |
| ----- | ---- |
| **Returns** | Simple returns between consecutive equity points: \(r_i = (V_i - V_{i-1}) / V_{i-1}\) when \(V_{i-1} > 0\). **Arithmetic**, not log. |
| **Annualization** | Calendar days between `BacktestResult::startDate` and `endDate`; years \(T = \text{days} / 365.25\). CAGR: \((1 + R)^{1/T} - 1\) for total return \(R\) when \(R > -1\). |
| **Trading-day Sharpe/Sortino** | Use **daily** returns derived from **ledger snapshots** (same series as `BacktestMetricsCollector`): consecutive snapshot portfolio values. **252** trading days per year for annualization of daily mean/std. |
| **Risk-free** | **0** unless otherwise configured (matches existing Sharpe in `BacktestMetricsCollector`). |
| **Missing periods** | Skip pairs with non-positive prior equity; do not impute. |
| **Zero trades** | Win rate / profit factor / expectancy: define as **0** or N/A; document per-field below. |

---

## Core metrics (aligned with `BacktestResult`)

- **totalReturn** — \((V_{end} - V_0) / V_0\) with \(V_0 = \) initial capital, \(V_{end}\) last snapshot or initial if empty.
- **annualizedReturn** — CAGR from total return over \(T\) years as above.
- **maxDrawdown** — Peak-to-trough on equity curve: max over \(t\) of \((\text{peak}_t - V_t) / \text{peak}_t\).
- **sharpeRatio** — \(\sqrt{252} \cdot \bar{r} / \sigma\) for daily simple returns; 0 if \(\sigma = 0\) or fewer than 2 returns.
- **winRate** — Legacy pairing in `BacktestMetricsCollector` (buy then sell same symbol); kept for compatibility until unified trade-P&L definition exists.

---

## Extended metrics (`BacktestStatistics`)

- **sortinoRatio** — \(\sqrt{252} \cdot \bar{r} / \sigma_d\) where \(\sigma_d\) is **downside** std of daily returns: use only returns **below 0**; if fewer than 2 downside points, fall back to **0** when \(\sigma_d = 0\).
- **calmarRatio** — annualizedReturn / maxDrawdown when maxDrawdown > epsilon; else 0.
- **profitFactor** — **Long-only FIFO realized P&L on sells** (matches `BacktestStatisticsCalculator`): gross wins / |gross losses| when losses exist; **1e6** if no losses but wins exist; **0** if no gross wins.
- **expectancy** — \((\text{finalCapital} - \text{initialCapital}) / \text{totalTrades}\) when totalTrades > 0; else **0**.
- **averageTradePnl** — Same as expectancy for v1.
- **averageExposurePct** — Mean over equity snapshots of \(\max(0, V - \text{cash}) / V\) when \(V = \) portfolioValue > 0. All cash ⇒ **0** exposure.
- **turnoverAnnualized** — \(\big(\sum_{\text{fills}} |q \cdot p| \big) / (2 \cdot \bar{V}) \cdot (252 / N)\) where \(\bar{V}\) is mean portfolio value over snapshots and \(N = \max(1, |\text{curve}| - 1)\) return intervals (same annualization scale as daily Sharpe).
- **monthlyReturns** / **yearlyReturns** — JSON arrays of `{ "period": "yyyy-MM" | "yyyy", "return": r }` using **first** and **last** snapshot in each calendar period; simple return \((V_{\text{last}} - V_{\text{first}}) / V_{\text{first}}\).

### Persistence

`DbBacktestMetrics` stores scalar extended fields plus **statisticsJson** (compact `BacktestStatistics::toJson(1)` including monthly/yearly arrays). Rows written before extended columns existed may have **NULL** extended fields; **recompute** from `BacktestResult` (trades + equity) with `BacktestStatisticsCalculator` when `metricDefinitionsVersion` is missing or stale vs current code.

---

## Benchmark policy (`benchmark-policy`)

| Topic | Rule |
| ----- | ---- |
| **Alignment** | Strategy and benchmark use **same** `startDate` / `endDate` from `BacktestResult`. |
| **Missing benchmark** | If `benchmark.equityCurve` empty, benchmark Sharpe/DD/returns come from **`BenchmarkResult`** scalar fields already computed by session; do not invent points. |
| **Timeline** | Benchmark CAGR/DD computed on **benchmark equity series** normalized to initial capital (as in session); strategy metrics on strategy curve — **not** mixed-timestamp without resampling. |
| **Cash drag** | Not modeled separately; benchmark is buy-and-hold per engine. **Out of scope** for v1 unless `BacktestResult` adds cash series. |

---

## JSON export versions

- **schemaVersion** — `1` for `BacktestStatistics` / `BacktestReportModel` JSON shape.
- **generatorVersion** — Application version string (`QCoreApplication::applicationVersion()` or `"dev"`).
