# Remaining Gaps: LEGO Pipeline Integration Status

**Last updated**: March 2026
**Status**: Phase D complete. CDispatcher has been fully removed. All data flows exclusively through typed Qt signal/slot routers. Legacy strategies have been replaced by LEGO pipeline equivalents.

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
| Execution details | Working | `IBComClientImpl::execDetails()` -> `OrderRouter::executionReceived()` -> legacy via adapter slots |
| Commission reports | Working | `IBComClientImpl::commissionReport()` -> `OrderRouter::commissionReceived()` -> legacy via adapter slots |
| Next valid ID | Working | `IBComClientImpl::nextValidId()` -> `OrderRouter` -> legacy via adapter slots |
| Account summary | Working | `IBComClientImpl::accountSummary()` -> `AccountRouter` -> legacy via adapter slots |
| Current time | Working | `IBComClientImpl::currentTime()` -> `TimeRouter` -> `AlphaModGetTime` |
| Infrastructure errors | Working | `IBComClientImpl::error()` -> `MarketDataRouter::subscriptionError/Restarted` |
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
| UI: Add strategy | Working | Unified "Add Strategy" creates pipeline strategies via config picker |
| UI: Edit parameters | Working | Flattened pipeline config editable in tree parameter editor |
| Supervision | Working | `Supervisor` monitors health, restarts crashed runtimes |
| Structured logging | Working | `StructuredLogger` with correlation IDs |
| Metrics collection | Working | `MetricsCollector` tracks ticks, orders, latency |
| JSON serialization | Working | Pipeline config saved/loaded in `model_tree_config.json` |

---

## Completed Migration Phases

### Phase A: Typed Routers (Complete)
All typed routers created and wired. `IBComClientImpl` forwards to typed routers.

### Phase B: Legacy Subscriber Migration (Complete)
4 message types migrated to typed routers: `RT_NEXT_VALID_ID`, `RT_REQ_ACCOUNT_SUMMURY`, `RT_REQ_POSITION`, `RT_HISTORICAL_DATA`.

### Phase C: Strategy Migration (Complete)
All legacy strategies removed. Only pipeline-based strategies can be created:
- `cMomentum`, `CMovingAverageCrossover`, `CTestStrategy` deleted
- `PairTrader`, `AutoDeltAlignment` deleted
- `slotOnClickAddStrategy()` now creates pipeline strategies exclusively
- Legacy `ModelType` entries removed from `CStrategyFactory`

### Phase D: CDispatcher Removal (Complete)
CDispatcher infrastructure fully retired:
- All `SendMessageToSubscribers` calls removed from `IBComClientImpl`
- `CSubscriber` inheritance removed from `CProcessingBase_v2`, `CPresenter`, `BaseImpl`, `AlphaModGetTime`
- `CBrokerDataProvider` no longer inherits `CDispatcher`; uses standalone `GlobalReqManager`
- `Dispatcher.h` / `Dispatcher.cpp` deleted
- `TimeRouter` created for current-time delivery
- Adapter slots in `CProcessingBase_v2` convert typed router data to legacy CObject types
- All request methods operate without subscriber parameters

---

## What Still Remains

### Minor Items (Low Priority)

1. **Visual pipeline builder UI** -- Currently uses JSON config + text parameter editing. A drag-and-drop block composer would improve UX but is not blocking.

2. **Rarely-used CDispatcher message types** -- `RT_TICK_GENERIC`, `RT_TICK_STRING`, `RT_HISTORICAL_TICK_DATA`, `RT_MKT_DEPTH`, `RT_MKT_DEPTH_L2`, `RT_REQ_OPTION_PRICE` no longer have CDispatcher forwarding. If any future block needs them, new typed router signals should be added.

3. **GlobalReqManager simplification** -- `GlobalReqManager` is now a standalone member of `CBrokerDataProvider`. It could be further simplified since subscriber-based ID tracking is no longer needed.

See `DISPATCHER_RETIREMENT_AUDIT.md` for the complete per-message-type migration status.
