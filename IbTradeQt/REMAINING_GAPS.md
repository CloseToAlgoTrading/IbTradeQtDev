# Remaining Gaps: LEGO Pipeline Integration Status

**Last updated**: March 2026
**Status**: Phase B complete. Legacy subscribers (`CBasicRoot`, `CBasicAccount`, `CBasicAlphaModel`) now receive data exclusively through typed routers. CDispatcher forwarding has been removed for 4 message types: `RT_NEXT_VALID_ID`, `RT_REQ_ACCOUNT_SUMMURY`, `RT_REQ_POSITION`, `RT_HISTORICAL_DATA`.

---

## What Works Today

| Capability | Status | How |
|-----------|--------|-----|
| Tick prices flow to pipeline | Working | `IBComClientImpl::tickPrice()` -> `MarketDataRouter::tick()` |
| Volume/tick size flow to pipeline | Working | `IBComClientImpl::tickSize()` -> `MarketDataRouter::onTickSize()` |
| Bar closes flow to pipeline | Working | `IBComClientImpl::realtimeBar()` -> `MarketDataRouter::barClose()` |
| Tick-by-tick trades | Working | `IBComClientImpl::tickByTickAllLast()` -> `MarketDataRouter::tickByTickTrade()` |
| Historical data | Working | `IBComClientImpl::historicalData()` -> `HistoricalDataRouter` -> legacy via adapter slots |
| Position updates | Working | `IBComClientImpl::position()` -> `PositionRouter` -> legacy via adapter slots |
| Order status | Working | `IBComClientImpl::orderStatus()` -> `OrderRouter::orderStatusChanged()` |
| Execution details | Working | `IBComClientImpl::execDetails()` -> `OrderRouter::executionReceived()` |
| Commission reports | Working | `IBComClientImpl::commissionReport()` -> `OrderRouter::commissionReceived()` |
| Next valid ID | Working | `IBComClientImpl::nextValidId()` -> `OrderRouter` -> legacy via adapter slots |
| Account summary | Working | `IBComClientImpl::accountSummary()` -> `AccountRouter` -> legacy via adapter slots |
| Pipeline runs on bar close | Working | `StrategyRuntime` triggers `StrategyPipelineRunner` on `barClose` |
| DryRun execution | Working | `MockExecutionAdapter` records orders locally |
| Live execution | Working | `IBOrderExecutionAdapter` -> `reqPlaceOrderAPI()` |
| Limit order execution | Working | `IBOrderExecutionAdapter` -> `reqPlaceLimitOrderAPI()` with LMT price |
| Stop order execution | Working | `IBOrderExecutionAdapter` -> `reqPlaceStopOrderAPI()` with STP price |
| Live position awareness | Working | `IBPositionRepositoryAdapter` via `PositionRouter`, thread-safe with `QMutex` |
| Persistent positions | Working | `SqlitePositionRepository` with WAL mode for DryRun state persistence |
| Pipeline config picker | Working | Scans `DefaultPipelines/` folder, presents selection dialog |
| Static list selection | Working | `StaticListSelectionBlock` filters universe by configured symbols |
| MA crossover alpha | Working | `MovingAverageCrossoverAlphaBlock` with fast/slow period crossover |
| Limit order block | Working | `LimitOrderExecutionBlock` for LMT orders |
| Order status feedback | Working | `OrderRouter` relays -> `IBOrderExecutionAdapter::updateOrderStatus()` |
| UI: Add pipeline strategy | Working | Config picker dialog with available pipeline configs |
| UI: Edit parameters | Working | Flattened pipeline config editable in tree parameter editor |
| Supervision | Working | `Supervisor` monitors health, restarts crashed runtimes |
| Structured logging | Working | `StructuredLogger` with correlation IDs |
| Metrics collection | Working | `MetricsCollector` tracks ticks, orders, latency |
| JSON serialization | Working | Pipeline config saved/loaded in `model_tree_config.json` |
| **Legacy nextValidId via router** | **Phase B** | `OrderRouter` -> `CProcessingBase_v2::slotRouterNextValidId()` |
| **Legacy account summary via router** | **Phase B** | `AccountRouter` -> `CProcessingBase_v2::slotRouterAccountSummary()` -> `signalRecvAccountSummary` |
| **Legacy positions via router** | **Phase B** | `PositionRouter` -> `CProcessingBase_v2::slotRouterPositionChanged()` -> `m_positionMap` |
| **Legacy historical data via router** | **Phase B** | `HistoricalDataRouter` -> `CProcessingBase_v2::slotRouterBarsReceived()` -> `signalCbkRecvHistoricalData` |

---

## Phase B: Legacy Subscriber Migration (Complete)

| Migration | What Changed | Files |
|-----------|-------------|-------|
| RT_NEXT_VALID_ID | `OrderRouter::nextValidIdReceived` -> `CProcessingBase_v2::slotRouterNextValidId()`, CDispatcher forwarding removed | `cprocessingbase_v2.h/.cpp`, `IBComClientIpml.cpp` |
| RT_REQ_ACCOUNT_SUMMURY | `AccountRouter::accountSummaryUpdated` -> adapter slot converts `AccountSummaryData` to `CAccountSummary` | `cprocessingbase_v2.h/.cpp`, `IBComClientIpml.cpp` |
| RT_REQ_POSITION | `PositionRouter::positionChanged/positionSnapshotComplete` -> adapter slots populate `m_positionMap` and emit `signalEndRecvPosition` | `cprocessingbase_v2.h/.cpp`, `IBComClientIpml.cpp` |
| RT_HISTORICAL_DATA | `HistoricalDataRouter::barsReceived` -> adapter slot converts `HistoricalBar` to `CHistoricalData`, emits `signalCbkRecvHistoricalData` | `cprocessingbase_v2.h/.cpp`, `cbrokerdataprovider.cpp`, `IBComClientIpml.cpp` |
| Router transport | Typed router pointers stored in `CBrokerDataProvider`, propagated to all models via `setBrokerDataProvider()` | `cbrokerdataprovider.h` |
| Controller wiring | Routers created and set on `CBrokerDataProvider` before `loadTreeFromFile()` | `capplicationcontroller.cpp` |

---

## What Still Remains

### Minor Items (Low Priority)

1. **Visual pipeline builder UI** -- Currently uses JSON config + text parameter editing. A drag-and-drop block composer would improve UX but is not blocking.

2. **Unmigrated CDispatcher message types** -- `RT_TICK_GENERIC`, `RT_TICK_STRING`, `RT_HISTORICAL_TICK_DATA`, `RT_MKT_DEPTH`, `RT_MKT_DEPTH_L2`, `RT_REQ_OPTION_PRICE`, `RT_ORDER_STATUS`, `RT_ORDER_COMMISSION`, `RT_ORDER_EXECUTION`, `RT_REALTIME_BAR`, `RT_TICK_PRICE`, `RT_TICK_SIZE`, `RT_TICK_BY_TICK_DATA` still forward through CDispatcher. Some (tick price, tick size, realtime bar, tick-by-tick) also forward through typed routers (dual-path). Others are rarely used.

3. **Full CDispatcher removal** -- CDispatcher still exists but 4 of its message types no longer forward through it. Legacy strategies still use `CProcessingBase_v2` request methods (e.g., `reqestRealTimeData`, `requestRealTimeBars`) which subscribe via CDispatcher for other message types.

4. **Legacy strategy side-by-side validation** -- `cMomentum` and `CMovingAverageCrossover` have LEGO equivalents but have not been run side-by-side to validate signal equivalence.

5. **Other legacy strategies** -- `csma`, `cteststrategy`, `PairTraderPM`, `AutoDeltAlignmentProcessing` have not been migrated.

See `DISPATCHER_RETIREMENT_AUDIT.md` for the complete per-message-type migration status.

---

## CDispatcher Retirement Roadmap

```
Phase A (Complete):
  All typed routers created and wired
  OrderEventBridge removed (replaced by OrderRouter)
  LEGO pipeline has full IB data access via typed routers
  ↓
Phase B (Complete):
  CProcessingBase_v2 connects to typed routers via CBrokerDataProvider
  Adapter slots convert Q_GADGET structs to legacy CObject types
  CDispatcher forwarding REMOVED for:
    RT_NEXT_VALID_ID, RT_REQ_ACCOUNT_SUMMURY,
    RT_REQ_POSITION, RT_HISTORICAL_DATA
  m_useTypedRouters guard prevents duplicate delivery
  ↓
Phase C: Migrate all strategies to LEGO blocks
  - Validate each strategy's signals match legacy
  - Use existing pipeline configs
  ↓
Phase D: Migrate remaining CDispatcher message types
  - RT_TICK_PRICE, RT_TICK_SIZE, RT_REALTIME_BAR (already dual-path)
  - RT_TICK_BY_TICK_DATA (already dual-path)
  - RT_ORDER_STATUS, RT_ORDER_COMMISSION, RT_ORDER_EXECUTION
  - Rarely-used types on demand
  ↓
Phase E: Remove CDispatcher
  - Only when ZERO subscribers remain
  - Remove CSubscriber, CDispatcher, GlobalReqManager
  - Remove CProcessingBase_v2::MessageHandler
  - Remove void* casting from CBrokerDataProvider
```
