# Backtest engine tests

## Default CI / `make check`

Runs **offline** tests only:

- CSV / JSONL data sources, ledger, metrics, replay
- **Mock Yahoo**: `TestYahooBacktestSessionMockE2E` exercises `dataSourceId=yahoo` with a fake `QNetworkAccessManager` (no network)
- `TestYahooBacktestSessionMockFailure` verifies load errors surface as `failed()`
- `TestYahooBacktestPipelineVariants` checks **benchmark buy-and-hold math** (total return, annualization vs `BenchmarkComparison`, `alphaVsBenchmark`, terminal equity), a **minimal pipeline JSON** (no `rebalance` / `risks` keys — factory defaults), and **MA + max-position-risk** with alternate fill timing
- `TestBacktestEngineCoverage` — **high-value engine matrix** (not a full Cartesian product): all `FillModelType` × `FillTiming` on CSV, slippage extremes, inverted CSV date window (no bars → initial-only metrics), empty universe / empty pipeline JSON failure paths, JSONL tick smoke, Yahoo mock malformed JSON + empty body (documents “soft” parse-empty behavior), `BacktestMetricsCollector` empty-curve and single-snapshot return formula, **`BacktestController` + temp SQLite + worker thread** with `BacktestRuns.status = Finished`, **`BacktestMetrics` row equality against `finished()` payload** (parity with UI), and **CSV benchmark metrics vs `BenchmarkComparison` reference** (total/annualized return, Sharpe, DD, start/end price, `alphaVsBenchmark`)

## Extended LEGO suite (automated, optional)

Multi-scenario backtests using **DefaultPipelines** JSON + synthetic CSV bars (no Yahoo HTTP):

```bash
cd tests/release && IBTRADING_EXTENDED_BACKTEST=1 ./ibtrading_tests
```

Set `IBTRADING_EXTENDED_BACKTEST=1` in a nightly job or run locally before release.

## Live Yahoo (real `query1.finance.yahoo.com`)

This is the **primary** automated path that uses **real** Yahoo JSON (not mocks) for the **long** MA-crossover backtests. Use it before release to validate parsing + network + full session on production-like data.

Semantic pipeline + `ModelDataList` rebalance path is covered in CI by **`TestSemanticE2E`** (mock ports / no Yahoo HTTP).

Requires network; can be flaky if Yahoo throttles or is down.

```bash
cd tests/release && IBTRADING_LIVE_TESTS=1 ./ibtrading_tests
```

`TestLiveBacktest` downloads historical data and may write HTML/TXT under `$TMPDIR/ibtrading_backtest_reports/`. Inspect:

- Equity curve length vs date range
- Benchmark vs strategy returns (signs and order of magnitude)
- Report files for manual sanity (charts/tables)

**Calculation coverage:** CI uses mock Yahoo + `TestBenchmarkComparison` (unit) + `TestYahooBacktestPipelineVariants` (integration formulas). Live runs add **end-to-end** validation against the live API, not duplicate formula proofs.

## Other live test

`liveYahooAmdNvdaVsSpyBenchmark` in `backtest/tst_yahoo_backtest.h` is skipped unless `IBTRADING_LIVE_TESTS=1`.

## Logging

```bash
QT_LOGGING_RULES="backtest.*=true" ./ibtrading_tests
```

## Yahoo multi-symbol ordering

`YahooFinanceDataSource` loads one symbol per HTTP response. `BacktestSession` buffers Yahoo bars and **sorts by timestamp** before feeding `MarketDataReplayer`, matching time-ordered CSV loads.

## HistoricalDataManager (SQLite) and prefetch

- **Table:** `HistoricalBars` stores daily bars keyed by `(symbol, resolution, dataSourceId, timestamp)`.
- **`getBarsMulti`:** Reads cached ranges, then **fills gaps** with Yahoo via `YahooFinanceDataSource`. For multiple symbols, missing segments are **unioned** into one `[fetchFrom, fetchTo]` per fetch wave (see `Backtest/HistoricalDataManager.cpp` — same segment rules as `getBars` for prefix / suffix / full range).
- **App backtest order:** `BacktestController` typically calls `getBarsMulti` for **strategy symbols** first, then **benchmark** (e.g. SPY) in a second call — so logs may show two prefetch waves.
- **Preload vs HTTP:** `BacktestSession` logs `using … preloaded bars` **after** prefetch. **`YahooFinanceDataSource: GET` lines appear during prefetch**, not because preload is ignored. If the cache already covers `[start, end]` for all symbols, `getBarsMulti` does **not** open new HTTP requests.
- **Second run / tests:** A **new temporary DB** per test (e.g. `QTemporaryFile`) has an **empty** cache every time — expect Yahoo GETs on every run. A **persistent** BacktestStore DB reuses rows and reduces network traffic when the range is unchanged.
- **Narrow `period1`/`period2` (one day):** Normal when only **one** calendar day is missing at the end (or start) of the cached range — incremental gap fill, not a full-history bug.
