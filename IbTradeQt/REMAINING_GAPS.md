# Remaining Gaps: LEGO Pipeline Integration Status

**Last updated**: March 2026
**Status**: All 8 original gaps have been addressed. The LEGO pipeline now has typed routers for every message type, limit/stop order support, persistent positions, a config picker UI, selection blocks, legacy strategy migration paths, and `OrderEventBridge` has been removed.

---

## What Works Today

| Capability | Status | How |
|-----------|--------|-----|
| Tick prices flow to pipeline | Working | `IBComClientImpl::tickPrice()` -> `MarketDataRouter::tick()` |
| Volume/tick size flow to pipeline | Working | `IBComClientImpl::tickSize()` -> `MarketDataRouter::onTickSize()` |
| Bar closes flow to pipeline | Working | `IBComClientImpl::realtimeBar()` -> `MarketDataRouter::barClose()` |
| **Tick-by-tick trades** | **NEW** | `IBComClientImpl::tickByTickAllLast()` -> `MarketDataRouter::tickByTickTrade()` |
| **Historical data** | **NEW** | `IBComClientImpl::historicalData()` -> `HistoricalDataRouter::historicalBar()` / `barsReceived()` |
| **Position updates** | **NEW** | `IBComClientImpl::position()` -> `PositionRouter::positionChanged()` |
| **Order status** | **NEW** | `IBComClientImpl::orderStatus()` -> `OrderRouter::orderStatusChanged()` |
| **Execution details** | **NEW** | `IBComClientImpl::execDetails()` -> `OrderRouter::executionReceived()` |
| **Commission reports** | **NEW** | `IBComClientImpl::commissionReport()` -> `OrderRouter::commissionReceived()` |
| **Next valid ID** | **NEW** | `IBComClientImpl::nextValidId()` -> `OrderRouter::nextValidIdReceived()` |
| **Account summary** | **NEW** | `IBComClientImpl::accountSummary()` -> `AccountRouter::accountSummaryUpdated()` |
| Pipeline runs on bar close | Working | `StrategyRuntime` triggers `StrategyPipelineRunner` on `barClose` |
| DryRun execution | Working | `MockExecutionAdapter` records orders locally |
| Live execution | Working | `IBOrderExecutionAdapter` -> `reqPlaceOrderAPI()` |
| **Limit order execution** | **NEW** | `IBOrderExecutionAdapter` -> `reqPlaceLimitOrderAPI()` with LMT price |
| **Stop order execution** | **NEW** | `IBOrderExecutionAdapter` -> `reqPlaceStopOrderAPI()` with STP price |
| **Live position awareness** | **NEW** | `IBPositionRepositoryAdapter` via `PositionRouter`, thread-safe with `QMutex` |
| **Persistent positions** | **NEW** | `SqlitePositionRepository` with WAL mode for DryRun state persistence |
| **Pipeline config picker** | **NEW** | Scans `DefaultPipelines/` folder, presents selection dialog |
| **Static list selection** | **NEW** | `StaticListSelectionBlock` filters universe by configured symbols |
| **MA crossover alpha** | **NEW** | `MovingAverageCrossoverAlphaBlock` with fast/slow period crossover |
| **Limit order block** | **NEW** | `LimitOrderExecutionBlock` for LMT orders |
| Order status feedback | Working | `OrderRouter` relays -> `IBOrderExecutionAdapter::updateOrderStatus()` |
| UI: Add pipeline strategy | Working | Config picker dialog with available pipeline configs |
| UI: Edit parameters | Working | Flattened pipeline config editable in tree parameter editor |
| Supervision | Working | `Supervisor` monitors health, restarts crashed runtimes |
| Structured logging | Working | `StructuredLogger` with correlation IDs |
| Metrics collection | Working | `MetricsCollector` tracks ticks, orders, latency |
| JSON serialization | Working | Pipeline config saved/loaded in `model_tree_config.json` |

---

## Gaps Closed in This Release

| Gap | What Was Fixed | Files Changed |
|-----|---------------|---------------|
| Gap 1: CDispatcher parallel routing | Typed routers for ALL 11 message types now run in parallel with CDispatcher | `HistoricalDataRouter.h`, `PositionRouter.h`, `OrderRouter.h`, `AccountRouter.h`, `MarketDataRouter.h` (tick-by-tick), `IBComClientIpml.cpp` |
| Gap 2: No live position repo | `IBPositionRepositoryAdapter` + `PositionRouter` provide live IB positions | `IBPositionRepositoryAdapter.h`, `PositionRouter.h`, `capplicationcontroller.cpp` |
| Gap 3: No selection block | `StaticListSelectionBlock` + `PipelineFactory` selection parsing | `StaticListSelectionBlock.h`, `PipelineFactory.h` |
| Gap 4: No config picker UI | `QInputDialog` selection of available pipeline configs | `portfolioconfigmodel.cpp`, `CPortfolioConfigModel.h` |
| Gap 5: No limit/stop orders | `reqPlaceLimitOrderAPI()` + `reqPlaceStopOrderAPI()` + `LimitOrderExecutionBlock` | `IBrokerAPI.h`, `IBComClientIpml.cpp`, `IBOrderExecutionAdapter.h`, `LimitOrderExecutionBlock.h` |
| Gap 6: Legacy migration | `MovingAverageCrossoverAlphaBlock` + migration JSON configs | `MovingAverageCrossoverAlphaBlock.h`, `*.json` configs |
| Gap 7: No tick-by-tick/historical | `TickByTickTrade` struct + `HistoricalDataRouter` + `IAlphaBlock::onTickByTick()` | `MarketDataRouter.h`, `HistoricalDataRouter.h`, `IAlphaBlock.h`, `StrategyRuntime.h` |
| Gap 8: SqlitePositionRepo not wired | `SqlitePositionRepository` with WAL mode, own DB connection | `SqlitePositionRepository.h`, `capplicationcontroller.cpp` |

---

## What Still Remains

### Minor Items (Low Priority)

1. **Visual pipeline builder UI** — Currently uses JSON config + text parameter editing. A drag-and-drop block composer would improve UX but is not blocking.

2. **Unmigrated CDispatcher message types** — `RT_TICK_GENERIC`, `RT_TICK_STRING`, `RT_HISTORICAL_TICK_DATA`, `RT_MKT_DEPTH`, `RT_MKT_DEPTH_L2`, `RT_REQ_OPTION_PRICE` are not routed. These are rarely used by current strategies and can be migrated on-demand.

3. **Full CDispatcher removal** — CDispatcher is still active (Phase A: dual-forwarding). Legacy strategies (`CBasicRoot`, `CBasicAccount`, `CBasicPortfolio`, `CProcessingBase_v2`) still consume from it. Removal requires migrating all remaining legacy subscribers.

4. **Legacy strategy side-by-side validation** — `cMomentum` and `CMovingAverageCrossover` have LEGO equivalents but have not been run side-by-side to validate signal equivalence.

5. **Other legacy strategies** — `csma`, `cteststrategy`, `PairTraderPM`, `AutoDeltAlignmentProcessing` have not been migrated.

See `DISPATCHER_RETIREMENT_AUDIT.md` for the complete per-message-type migration status.

---

## CDispatcher Retirement Roadmap

```
Current state (Phase A - Complete):
  ✅ All typed routers created and wired (dual-path with CDispatcher)
  ✅ PositionRouter, HistoricalDataRouter, OrderRouter, AccountRouter
  ✅ MarketDataRouter extended with tick-by-tick
  ✅ OrderEventBridge removed (replaced by OrderRouter)
  ✅ LEGO pipeline has full IB data access via typed routers
  ↓
Phase B: Migrate remaining legacy subscribers
  - CBasicRoot: subscribe to OrderRouter::nextValidIdReceived instead of RT_NEXT_VALID_ID
  - CBasicAccount: subscribe to AccountRouter instead of RT_REQ_ACCOUNT_SUMMURY
  - CBasicPortfolio: subscribe to PositionRouter instead of RT_REQ_POSITION
  - CBasicAlphaModel: subscribe to HistoricalDataRouter instead of RT_HISTORICAL_DATA
  - Migrate one subscriber at a time
  ↓
Phase C: Migrate all strategies to LEGO blocks
  - Validate each strategy's signals match legacy
  - Use existing pipeline configs (legacy_momentum_pipeline.json, ma_crossover_pipeline.json)
  ↓
Phase D: Remove CDispatcher
  - Only when ZERO subscribers remain
  - Remove CSubscriber, CDispatcher, GlobalReqManager
  - Remove CProcessingBase_v2::MessageHandler
  - Remove void* casting from CBrokerDataProvider
```
