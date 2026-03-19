# Remaining Gaps: LEGO Pipeline Integration Status

**Last updated**: March 2026
**Status**: Phase E complete. All 6 remaining message types now have full typed router signals. CDispatcher has been fully removed. All data flows exclusively through typed Qt signal/slot routers. Legacy strategies have been replaced by LEGO pipeline equivalents.

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
| Generic ticks | Working | `IBComClientImpl::tickGeneric()` -> `MarketDataRouter::tickGenericReceived()` |
| String ticks | Working | `IBComClientImpl::tickString()` -> `MarketDataRouter::tickStringReceived()` |
| Option computation | Working | `IBComClientImpl::tickOptionComputation()` -> `MarketDataRouter::optionComputationReceived()` |
| Market depth (L1) | Working | `IBComClientImpl::updateMktDepth()` -> `MarketDepthRouter::depthUpdated()` |
| Market depth (L2) | Working | `IBComClientImpl::updateMktDepthL2()` -> `MarketDepthRouter::depthL2Updated()` |
| Historical tick data | Working | `IBComClientImpl::historicalTicksLast()` -> `HistoricalDataRouter::historicalTicksLastReceived()` |
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

### Phase E: Remaining Router Signals (Complete)
All 6 previously-unused message types now have full typed router coverage:
- `tickGeneric` / `tickString` / `tickOptionComputation` -> `MarketDataRouter` (new `GenericTick`, `StringTick`, `OptionComputation` Q_GADGET structs)
- `updateMktDepth` / `updateMktDepthL2` -> new `MarketDepthRouter` (new `DepthUpdate`, `DepthL2Update` Q_GADGET structs)
- `historicalTicksLast` -> `HistoricalDataRouter` (new `HistoricalTickLast` Q_GADGET struct)
- 7 new integration tests in `tst_phase_e_remaining_routers.h`

---

## What Still Remains

### Minor Items (Low Priority)

1. **Visual pipeline builder UI** -- Currently uses JSON config + text parameter editing. A drag-and-drop block composer would improve UX but is not blocking.

2. **GlobalReqManager simplification** -- `GlobalReqManager` is now a standalone member of `CBrokerDataProvider`. It could be further simplified since subscriber-based ID tracking is no longer needed.

3. **Legacy `strategy_definitions` removal** -- The old `strategy_definitions` table has been renamed to `strategy_definitions_backup` and an empty compatibility table recreated. Once the v3 catalog is proven stable, both can be dropped along with the legacy API wrappers in `ISystemBackend`.

See `DISPATCHER_RETIREMENT_AUDIT.md` for the complete per-message-type migration status.

---

## Recently Completed

### UI Decoupling and Coordinator Refactor (March 2026)

- **CPresenter decomposition**: Extracted `BacktestWorkspaceCoordinator` and `StrategyManagementCoordinator`; CPresenter is now a thin router (~455 lines vs ~1366 before)
- **ViewModels (DTOs)**: `SharedUI/ViewModels.h` with `VM::WorkspaceHeader`, `VM::ParameterRow`, `VM::TradeRow`, etc.
- **Per-workspace presenters**: `StrategyWorkspacePresenter`, `BacktestPresenter`, `BlockInspectorPresenter`, `StrategyDetailPresenter`, `PipelineDiagramModel`
- **Live vs Strategy Management split**: Add/Remove block context menu actions removed from Live Trading tree; only Strategy Management tab allows block composition
- **Tree view styling**: QSS-based finance font (Consolas/monospace 11px), row height 22px, `StrategyTreeDelegate` applies `opt.font` from QSS
- **Tab widget styling**: All styles in `operations-console.qss` via `QTabWidget#MainTabWidget` selector; no inline `setStyleSheet` in code

See [UI_DECOUPLING.md](UI_DECOUPLING.md) for full architecture and diagrams.

### Strategy Catalog and Management (March 2026)

Two-stage implementation of a dedicated Strategy Management tab:

**Stage A (Backend):**
- `strategies` + `strategy_versions` two-table schema (v3)
- Conservative v2→v3 migration with `strategy_definitions_backup` retention
- Backend catalog API: family CRUD, version CRUD, binding, divergence detection
- Backtest runs record `catalogStrategyId` + `catalogVersionId`
- Gated versioning: `detectVersionDivergence()` detects drift without auto-creating versions

**Stage B (UI + Integration):**
- Strategy Management tab with `QAbstractItemModel`-based catalog tree and detail panel
- Combined search + status filtering via custom `CatalogFilterProxy`
- Version config viewer with "Diff vs Previous" toggle
- Live tree integration: "Use Existing Strategy" creates nodes bound to existing catalog entries
- Backtest integration: catalog version picker in `BacktestStrategySelector`
- Gated versioning prompts before live deployment and after backtest runs
- Comprehensive test suite covering repository CRUD, migration, backend API, model population, sorting, filtering, divergence detection, and full lifecycle
