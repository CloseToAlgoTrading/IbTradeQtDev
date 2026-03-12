# Remaining Gaps: What Still Prevents Full LEGO Pipeline Use

**Last updated**: March 2026
**Status**: The LEGO pipeline runs end-to-end alongside legacy strategies. It can receive live ticks, generate signals, and place orders (DryRun or Live). However, several gaps remain before the legacy `CDispatcher` can be retired.

---

## What Works Today

| Capability | Status | How |
|-----------|--------|-----|
| Tick prices flow to pipeline | Working | `IBComClientImpl::tickPrice()` -> `MarketDataRouter::tick()` |
| Volume/tick size flow to pipeline | Working | `IBComClientImpl::tickSize()` -> `MarketDataRouter::onTickSize()` |
| Bar closes flow to pipeline | Working | `IBComClientImpl::realtimeBar()` -> `MarketDataRouter::barClose()` |
| Pipeline runs on bar close | Working | `StrategyRuntime` consumes ticks from `BoundedQueue`, triggers `StrategyPipelineRunner` on `barClose` |
| DryRun execution | Working | `MockExecutionAdapter` records orders locally |
| Live execution | Working | `IBOrderExecutionAdapter` -> `reqPlaceOrderAPI()` when execution_mode = "live" |
| Order status feedback | Working | `OrderEventBridge` relays `orderStatus()` / `execDetails()` -> `IBOrderExecutionAdapter::updateOrderStatus()` |
| UI: Add pipeline strategy | Working | "Add Pipeline Strategy (LEGO)" in tree context menu |
| UI: Edit parameters | Working | Flattened pipeline config editable in tree parameter editor |
| UI: Remove pipeline strategy | Working | `PM_ITEM_PIPELINE_STRATEGY` recognized in all tree operations |
| Supervision | Working | `Supervisor` monitors health, restarts crashed runtimes |
| Structured logging | Working | `StructuredLogger` with correlation IDs |
| Metrics collection | Working | `MetricsCollector` tracks ticks, orders, latency |
| JSON serialization | Working | Pipeline config saved/loaded in `model_tree_config.json` |
| 264 tests passing | Working | 17 test suites, all green |

---

## Remaining Gaps

### Gap 1: CDispatcher Still Carries 13+ Message Types

`MarketDataRouter` only handles 3 message types: tick prices, tick sizes, and bar closes. The remaining 13+ message types still flow **exclusively** through `CDispatcher`:

| Message Type | What It Carries | Who Needs It |
|---|---|---|
| `RT_HISTORICAL_DATA` | Historical OHLCV bars | `CBasicAlphaModel`, `AutoDeltAlignmentProcessing`, `PairTraderPM` |
| `RT_REQ_POSITION` | Live portfolio positions | `CBasicPortfolio`, `AutoDeltAlignmentProcessing` |
| `RT_ORDER_EXECUTION` | Execution fill reports | `CBasicExecutionModel` |
| `RT_ORDER_COMMISSION` | Commission data | `CBasicExecutionModel` |
| `RT_REQ_ACCOUNT_SUMMURY` | Account balances/NAV | `CBasicAccount` |
| `RT_MKT_DEPTH` / `RT_MKT_DEPTH_L2` | Order book L1/L2 | Currently unused by strategies |
| `RT_TICK_BY_TICK_DATA` | Tick-by-tick trades | `PairTraderPM` |
| `RT_REQ_OPTION_PRICE` | Option Greeks/prices | `AutoDeltAlignmentProcessing` |
| `RT_NEXT_VALID_ID` | Order ID seed | `CBasicRoot`, `CPresenter` |
| `RT_REQ_RESTART_SUBSCRIPTION` | Reconnection signal | Infrastructure |
| `RT_REQ_ERROR_SUBSRIPTION` | Subscription errors | Infrastructure |

**Impact**: The LEGO pipeline can run new strategies using tick/bar data. But it cannot replace legacy strategies that depend on historical data, positions, account summaries, or option pricing -- those still need `CDispatcher`.

**Fix**: Create typed routers for each remaining message type (one at a time):
- `HistoricalDataRouter` for `RT_HISTORICAL_DATA`
- `OrderRouter` for `RT_ORDER_STATUS`, `RT_ORDER_EXECUTION`, `RT_ORDER_COMMISSION`, `RT_NEXT_VALID_ID`
- `PositionRouter` for `RT_REQ_POSITION`
- `AccountRouter` for `RT_REQ_ACCOUNT_SUMMURY`
- Extend `MarketDataRouter` for `RT_TICK_BY_TICK_DATA`

### Gap 2: No Live Position Repository

`CPipelineStrategyAdapter` currently uses `MockPositionRepository` even in `Live` mode (for the position repo, not execution). The pipeline tracks positions internally via execution results, but there's no adapter that listens to IB `position()` callbacks.

**Impact**: Pipeline strategies don't know about positions from other strategies or manual trades. Position queries return mock data.

**Fix**: Create `IBPositionRepositoryAdapter` that:
- Subscribes to `IBComClientImpl::position()` callbacks
- Maintains a live position map
- Implements `IPositionRepositoryPort`

### Gap 3: No SelectionBlock Using IB Universe

The only `ISelectionBlock` implementation is `PassAllSelectionBlock` (accepts any symbol). There's no block that queries IB for available contracts or filters based on market data availability.

**Impact**: Pipeline strategies must have their symbol universe hard-coded in the JSON config. No dynamic universe selection.

**Fix**: Create `IB_ContractSelectionBlock` that queries available contracts from IB, or a `StaticListSelectionBlock` that reads symbols from config (simpler, sufficient for most use cases).

### Gap 4: No Pipeline Strategy Configuration UI

When "Add Pipeline Strategy (LEGO)" is clicked, it always loads `simple_momentum_pipeline.json`. There's no UI to:
- Choose which default pipeline config to load
- Browse/select from available pipeline configs
- Visually compose blocks (drag-and-drop)

**Impact**: Users must manually edit JSON or tree parameters to customize pipelines. No visual pipeline builder.

**Fix (incremental)**:
1. Add a dropdown in the context menu to choose from available JSON configs in `Strategies/DefaultPipelines/`
2. Add a dialog for basic pipeline configuration (select alpha, risk, execution blocks)
3. (Future) Visual block graph editor

### Gap 5: No Limit/Stop Order Support

`MarketOrderExecutionBlock` only places market orders. `ExecutionIntent` has `Limit` and `Stop` order types defined, but `IBOrderExecutionAdapter::placeOrder()` always calls `reqPlaceOrderAPI()` with just symbol/quantity/action -- no price or order type.

**Impact**: All pipeline orders are market orders. No support for limit orders, stop losses, or other order types.

**Fix**: Extend `IBOrderExecutionAdapter::placeOrder()` to pass `orderType` and `limitPrice` to a more detailed `reqPlaceOrderAPI()` overload.

### Gap 6: Legacy Strategies Not Yet Migrated

The three adapters (`AlphaModelAdapter`, `RiskModelAdapter`, `ExecutionModelAdapter`) exist to wrap legacy sub-models as LEGO blocks, but no legacy strategy has actually been migrated. The migration path is:

1. Wrap each legacy sub-model in its adapter
2. Create a pipeline JSON config that references the adapted blocks
3. Run both legacy and pipeline versions side-by-side
4. Validate output equivalence
5. Remove the legacy version

**Impact**: Legacy strategies (Momentum, MA Crossover, Pair Trader, AutoDelta) still run exclusively through `CDispatcher`.

### Gap 7: No Tick-by-Tick or Historical Data in Pipeline

Pipeline blocks can only consume `MarketTick` (bid/ask/volume) and `barClose` signals. There's no way for a pipeline block to:
- Request historical data bars
- Subscribe to tick-by-tick (last trade) data
- Access order book depth

**Impact**: Strategies that need historical lookback windows or tick-by-tick data cannot be implemented as LEGO blocks yet.

**Fix**: Extend `MarketDataRouter` with historical data request/response and tick-by-tick forwarding from `IBComClientImpl`.

### Gap 8: SqlitePositionRepository Not Wired

`SqlitePositionRepository` is implemented but never instantiated in the application. `CApplicationController` uses `MockPositionRepository` as the global position repo.

**Impact**: Pipeline position state is not persisted across application restarts.

**Fix**: Replace `MockPositionRepository` with `SqlitePositionRepository` in `CApplicationController`, wired to the existing `DBHandler`.

---

## CDispatcher Retirement Roadmap

```
Current state (Phase A):
  ✅ Dual-path for ticks + bars + tick sizes
  ✅ CDispatcher still active for everything
  ✅ LEGO pipeline runs alongside for tick/bar data
  ✅ Live execution wired via IBOrderExecutionAdapter + OrderEventBridge
  ✅ Default pipeline models (SimpleMomentum, DualAlpha) proven
  ↓
Phase B: Add typed routers for remaining message types
  - HistoricalDataRouter    (RT_HISTORICAL_DATA)
  - OrderRouter             (RT_ORDER_STATUS, RT_ORDER_EXECUTION, RT_ORDER_COMMISSION)
  - PositionRouter          (RT_REQ_POSITION)
  - AccountRouter           (RT_REQ_ACCOUNT_SUMMURY)
  - Extend MarketDataRouter (RT_TICK_BY_TICK_DATA)
  Each router added one at a time, with dual-path verification
  ↓
Phase C: Migrate legacy strategies to LEGO blocks
  - Use AlphaModelAdapter, RiskModelAdapter, ExecutionModelAdapter
  - Migrate one strategy at a time, validate equivalence
  ↓
Phase D: Remove CDispatcher
  - Only when ZERO subscribers remain
  - Remove CSubscriber, CDispatcher, GlobalReqManager
  - Remove CProcessingBase_v2::MessageHandler
  - Remove void* casting
  - Clean up CBrokerDataProvider to only use typed routers
```

### What makes Phase D safe

Each prior phase is **reversible**. If a typed router has a bug, the `CDispatcher` path still works as fallback. Only when every message type is confirmed working through the new typed path do we cut the `CDispatcher` wires.

---

## Priority Order

| Priority | Gap | Effort | Impact |
|----------|-----|--------|--------|
| 1 | Gap 8: Wire SqlitePositionRepository | Small | Position persistence across restarts |
| 2 | Gap 2: Live position repository adapter | Medium | Accurate position awareness |
| 3 | Gap 4: Pipeline config selection UI | Small | User can choose from available pipelines |
| 4 | Gap 5: Limit/stop order support | Medium | Non-market order types |
| 5 | Gap 1: Typed routers for remaining messages | Large | Prerequisite for CDispatcher retirement |
| 6 | Gap 7: Historical/tick-by-tick in pipeline | Large | Enables complex strategies |
| 7 | Gap 3: Dynamic selection block | Small | Dynamic universe |
| 8 | Gap 6: Migrate legacy strategies | Large | Full transition to LEGO |
