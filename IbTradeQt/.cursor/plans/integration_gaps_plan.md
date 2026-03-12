# Integration Gaps Plan: Wiring the LEGO Pipeline into the Live Application

**Status**: Planning  
**Prerequisite**: Phases 1–6 complete (224 tests passing)  
**Goal**: Bridge the new pipeline architecture into the existing application so it can run alongside (and eventually replace) the legacy CDispatcher path — without breaking anything.

---

## Current State

The new pipeline architecture exists **entirely in isolation**:
- `MarketDataRouter`, `StrategyPipelineRunner`, `StrategyRuntime`, `Supervisor`, all blocks, adapters, logging, metrics — all tested with mocks (224 tests)
- **None of these components are instantiated in the real application**
- The legacy `CDispatcher → void* → CSubscriber → MessageHandler` path is still the only live path

### Identified Gaps

| # | Gap | Severity | Where |
|---|-----|----------|-------|
| G1 | `MarketDataRouter` never created in app | Critical | `CPresenter` |
| G2 | `IBComClientImpl::m_marketDataRouter` always `nullptr` | Critical | `CPresenter` → `IBComClientImpl` |
| G3 | `registerSymbolForReqId()` never called — `m_reqIdToSymbol` empty | Critical | `CBrokerDataProvider` subscription flow |
| G4 | `realtimeBar()` not forwarded to `MarketDataRouter` | Medium | `IBComClientImpl::realtimeBar()` |
| G5 | `MarketOrderExecutionBlock` has no `IOrderExecutionPort` in production | Medium | Block wiring |
| G6 | `IBOrderExecutionAdapter` exists but never instantiated | Medium | App startup |
| G7 | No `Supervisor`/`StrategyRuntime` in the app | Critical | `CApplicationController`/`CPresenter` |
| G8 | Pipeline orders placed = 0 in benchmarks (dry-run mode) | Medium | Execution block wiring |

---

## Fix Plan

### Step 1: Symbol Registration Hook

**Why**: The bridge in `IBComClientImpl::tickPrice()` already checks `m_reqIdToSymbol.contains(tickerId)` — but the map is empty because nobody populates it.

**What to change**:

**File**: `IBComm/cbrokerdataprovider.h` / `.cpp`

When `CBrokerDataProvider` creates market data subscriptions (in `reqestRealTimeData`, `requestRealTimeBars`, etc.), it knows both the `reqId` and the `symbol`. After calling `getClien()->reqRealTimeDataAPI(reqId, config)`, add:

```cpp
// After reqRealTimeDataAPI call
auto* implClient = qobject_cast<IBComClientImpl*>(getClien().data());
if (implClient) {
    implClient->registerSymbolForReqId(curReq.id, _symbol);
}
```

**Alternative (cleaner)**: Add `registerSymbolForReqId(qint32, QString)` to `IBrokerAPI` as a virtual method with a default empty implementation. Override in `IBComClientImpl`. This avoids the `qobject_cast`.

**Files to modify**:
- `IBComm/IBrokerAPI.h` — add virtual `registerSymbolForReqId()` with default no-op
- `IBComm/IBComClientImpl.h` — already has it, mark as `override`
- `IBComm/cbrokerdataprovider.cpp` — call `registerSymbolForReqId()` in each subscription method

**Tests**: Unit test that after calling `reqestRealTimeData()`, the reqId→symbol mapping exists.

---

### Step 2: Wire MarketDataRouter in CPresenter

**Why**: The router must exist for the pipeline to receive ticks.

**What to change**:

**File**: `MainSystem/cpresenter.h` / `.cpp`

In `CPresenter` constructor, after creating `IBComClientImpl` and setting the client:

```cpp
// Create MarketDataRouter
m_pMarketDataRouter = new IBComm::MarketDataRouter(this);

// Wire it to IBComClientImpl
auto* implClient = qobject_cast<IBComClientImpl*>(m_pDataProvider->getClien().data());
if (implClient) {
    implClient->setMarketDataRouter(m_pMarketDataRouter);
}
```

Add member: `IBComm::MarketDataRouter* m_pMarketDataRouter = nullptr;`  
Add accessor: `IBComm::MarketDataRouter* marketDataRouter() const;`

**Files to modify**:
- `MainSystem/cpresenter.h` — add member + accessor
- `MainSystem/cpresenter.cpp` — create and wire in constructor

**Tests**: Integration test that ticks from `IBComClientImpl::tickPrice()` reach `MarketDataRouter::tick` signal.

---

### Step 3: Forward realtimeBar to MarketDataRouter

**Why**: The pipeline uses `barClose` signals to trigger pipeline runs. Currently `realtimeBar()` only dispatches via `CDispatcher`.

**What to change**:

**File**: `IBComm/IBComClientIpml.cpp`

In `IBComClientImpl::realtimeBar()`, after the existing `SendMessageToSubscribers` call:

```cpp
if (m_marketDataRouter && m_reqIdToSymbol.contains(reqId)) {
    QDateTime barTime = QDateTime::fromSecsSinceEpoch(time);
    m_marketDataRouter->onBarComplete(reqId, m_reqIdToSymbol[reqId], barTime);
}
```

**Files to modify**:
- `IBComm/IBComClientIpml.cpp` — add MarketDataRouter forwarding in `realtimeBar()`

**Tests**: Unit test that `realtimeBar()` triggers `MarketDataRouter::barClose`.

---

### Step 4: Wire Execution Adapter

**Why**: `MarketOrderExecutionBlock` has `setExecutionPort()` but nobody calls it. Orders never reach the broker.

**What to change**:

When creating a `BlockGraph` for live trading:

```cpp
auto* executionAdapter = new Adapters::IBOrderExecutionAdapter(
    m_pDataProvider->getClien().data());

auto* executionBlock = new Blocks::MarketOrderExecutionBlock();
executionBlock->setExecutionPort(executionAdapter);
graph.executionBlock = executionBlock;
```

**Where**: This happens wherever `BlockGraph` instances are built for live strategies — either in `CApplicationController`, a future strategy configuration UI, or a factory method.

**Files to modify**:
- The integration point (Step 6) where strategies are created

**Tests**: Already covered by existing `tst_pipeline_runner.h` tests. Add integration test that verifies `IBOrderExecutionAdapter::placeOrder()` calls through to `IBrokerAPI::reqPlaceOrderAPI()`.

---

### Step 5: Create Supervisor in Application

**Why**: The Supervisor manages strategy lifecycles (start, stop, crash recovery). It needs to exist in the running app.

**What to change**:

**File**: `MainSystem/capplicationcontroller.h` / `.cpp`

Add a `Supervision::Supervisor` member to `CApplicationController`. Create it during `setUpApplication()`:

```cpp
m_pSupervisor = new Supervision::Supervisor(this);
m_pSupervisor->startMonitoring(10000); // 10s health checks
```

Expose it so the UI or other components can add/remove strategy runtimes:

```cpp
Supervision::Supervisor* supervisor() const { return m_pSupervisor; }
```

**Files to modify**:
- `MainSystem/capplicationcontroller.h` — add member + accessor
- `MainSystem/capplicationcontroller.cpp` — create in `setUpApplication()`

**Tests**: Verify Supervisor is created and monitoring starts.

---

### Step 6: Strategy Creation Factory for LEGO Pipelines

**Why**: There's currently no way to create a LEGO pipeline strategy from the app. The existing `CStrategyFactory` only creates legacy `CBasicStrategy_V2` and its subclasses.

**What to change**:

Create a new factory that builds `BlockGraph` + `StrategyRuntime` from configuration:

**New file**: `Pipeline/PipelineFactory.h`

```cpp
namespace Pipeline {

class PipelineFactory {
public:
    static Supervision::StrategyRuntime* createFromConfig(
        const QJsonObject& config,
        IBComm::MarketDataRouter* router,
        Ports::IOrderExecutionPort* executionPort,
        Ports::IPositionRepositoryPort* positionRepo);
    
    static BlockGraph buildGraph(const QJsonObject& config);
};

} // namespace Pipeline
```

This uses `BlockRegistry` to look up blocks by ID and `BlockGraphSerializer` to deserialize configurations. It creates a `StrategyRuntime` and calls `connectToMarketData(router)`.

**Usage from CApplicationController**:

```cpp
auto* runtime = Pipeline::PipelineFactory::createFromConfig(
    strategyConfig, 
    m_pPresenter->marketDataRouter(),
    executionAdapter,
    positionRepo);
m_pSupervisor->addStrategy(name, [=]() {
    return Pipeline::PipelineFactory::createFromConfig(
        strategyConfig, router, executionPort, positionRepo);
}, Supervision::RestartPolicy::OnFailure);
```

**Files to create**:
- `Pipeline/PipelineFactory.h`

**Tests**: Create a `StrategyRuntime` from JSON config, feed it mock ticks, verify orders flow.

---

### Step 7: Dual-Path Operation (Coexistence)

**Why**: We can't rip out the legacy path in one shot. Both paths must coexist.

**Architecture during transition**:

```
IB TWS
  │
  ▼
IBComClientImpl::tickPrice()
  ├──► CDispatcher → Legacy strategies (unchanged)
  └──► MarketDataRouter → LEGO pipeline strategies (new)
```

**Key principle**: The `tickPrice()` bridge code already does this — it sends to both `CDispatcher` and `MarketDataRouter`. Steps 1-3 just activate the MarketDataRouter side.

**No legacy code changes needed** for the strategies themselves. They continue to work through `CDispatcher` as before.

---

## Execution Order

```
Step 1 → Step 2 → Step 3 → Step 4 → Step 5 → Step 6 → Step 7
  │         │         │         │         │         │
  │         │         │         │         │         └─ Factory for creating pipelines from config
  │         │         │         │         └─ Supervisor in app startup
  │         │         │         └─ Execution adapter wired to blocks
  │         │         └─ realtimeBar → barClose forwarding
  │         └─ MarketDataRouter created and connected
  └─ Symbol→reqId populated on subscription
```

Steps 1–3 can be done together (they activate the data flow).  
Steps 4–5 can be done together (they activate execution + supervision).  
Step 6 enables creating strategies from configuration.  
Step 7 is implicit — the dual-path is already designed in.

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Breaking legacy strategies | Dual-path: legacy `CDispatcher` path untouched; MarketDataRouter is additive |
| `qobject_cast` fails | Add `registerSymbolForReqId` to `IBrokerAPI` as virtual no-op instead |
| Thread safety (MarketDataRouter on IB thread) | MarketDataRouter emits signals; Qt::QueuedConnection delivers to strategy threads safely |
| Memory leaks from new objects | Supervisor owns runtimes; runtimes own runners; Qt parent-child for cleanup |
| realtimeBar reqId not in symbol map | Only forward if `m_reqIdToSymbol.contains(reqId)` — same guard as tickPrice |

---

## Deliverables per Step

| Step | Files Modified | Files Created | Tests Added |
|------|---------------|---------------|-------------|
| 1 | `IBrokerAPI.h`, `IBComClientImpl.h`, `cbrokerdataprovider.cpp` | — | Symbol registration test |
| 2 | `cpresenter.h`, `cpresenter.cpp` | — | Router wiring test |
| 3 | `IBComClientIpml.cpp` | — | realtimeBar forwarding test |
| 4 | — | — | Execution adapter integration test |
| 5 | `capplicationcontroller.h`, `capplicationcontroller.cpp` | — | Supervisor creation test |
| 6 | — | `Pipeline/PipelineFactory.h` | Factory integration test |
| 7 | — | — | End-to-end dual-path test |

---

## Step 8: Default LEGO Pipeline Models

**Why**: Prove the full integration path works with concrete, runnable examples before touching any legacy code.

**Models to create**:

### 8a. SimpleMomentumStrategy (pipeline config)

A minimal but complete LEGO pipeline using blocks we already built:

```
Selection: PassAllSelectionBlock (accept all universe symbols)
Alpha:     MomentumAlphaBlock (period=20, threshold=0.02)
Rebalance: SimpleRebalanceBlock (convert signals → target positions)
Risk:      MaxPositionRiskBlock (maxPositionSize=1000, maxTotalExposure=5000)
Execution: MarketOrderExecutionBlock (with IOrderExecutionPort wired)
```

**File**: `Strategies/DefaultPipelines/simple_momentum_pipeline.json`

This JSON file is loadable by `BlockGraphSerializer::fromJsonFile()` and `PipelineFactory::createFromConfig()`.

### 8b. MeanReversionAlphaBlock (new block)

A second alpha block to demonstrate multi-alpha + merge:

```
Alpha logic: If price is >2 std deviations above rolling mean → Sell
             If price is >2 std deviations below rolling mean → Buy
```

**File**: `Blocks/MeanReversionAlphaBlock.h`

### 8c. DualAlphaStrategy (pipeline config)

Uses two alphas (Momentum + MeanReversion) with WeightedVoteMerge:

```
Selection: PassAllSelectionBlock
Alpha[0]:  MomentumAlphaBlock
Alpha[1]:  MeanReversionAlphaBlock
Merge:     WeightedVoteMergePolicy
Rebalance: SimpleRebalanceBlock
Risk:      MaxPositionRiskBlock
Execution: MarketOrderExecutionBlock
```

**File**: `Strategies/DefaultPipelines/dual_alpha_pipeline.json`

### 8d. Integration test: full path validation

Test that creates each default pipeline from JSON, feeds 1000 simulated ticks through MarketDataRouter, verifies:
- Alpha blocks receive ticks and generate signals
- Pipeline runs produce target positions
- Risk blocks filter appropriately
- Execution block places orders through mock adapter
- StructuredLogger records correlation IDs end-to-end
- MetricsCollector tracks all operations

**File**: `tests/integration/tst_default_pipelines.h`

---

## Step 9: CDispatcher Retirement Roadmap

**Why CDispatcher cannot be removed yet:**

CDispatcher carries **15+ message types**. MarketDataRouter only handles 2 (tick prices, bar closes). The following still flow exclusively through CDispatcher:

| Message Type | What It Carries | Replacement Needed |
|---|---|---|
| `RT_HISTORICAL_DATA` | Historical OHLCV bars | `HistoricalDataRouter` (new) |
| `RT_REQ_POSITION` | Live portfolio positions | `PositionRouter` (new) |
| `RT_ORDER_EXECUTION` | Execution fill reports | `OrderRouter` (new) |
| `RT_ORDER_COMMISSION` | Commission data | `OrderRouter` (new) |
| `RT_REQ_ACCOUNT_SUMMURY` | Account balances/NAV | `AccountRouter` (new) |
| `RT_TICK_SIZE` | Volume/size data | Extend `MarketDataRouter` |
| `RT_MKT_DEPTH` | Order book L1 | `MarketDepthRouter` (new) |
| `RT_TICK_BY_TICK_DATA` | Tick-by-tick trades | Extend `MarketDataRouter` |
| `RT_REQ_OPTION_PRICE` | Option Greeks/prices | `OptionDataRouter` (new) |
| `RT_NEXT_VALID_ID` | Order ID seed | `OrderRouter` (new) |
| `RT_REQ_RESTART_SUBSCRIPTION` | Reconnection | Infrastructure signal |
| `RT_REQ_ERROR_SUBSRIPTION` | Subscription errors | Infrastructure signal |

### Retirement phases:

```
Phase A: Dual-path for ticks+bars (Steps 1-8 above)
         CDispatcher still active for everything
         LEGO pipeline runs alongside for tick/bar data
         ↓
Phase B: Add typed routers for each remaining message type
         - HistoricalDataRouter    (RT_HISTORICAL_DATA)
         - OrderRouter             (RT_ORDER_STATUS, RT_ORDER_EXECUTION, RT_ORDER_COMMISSION, RT_NEXT_VALID_ID)
         - PositionRouter          (RT_REQ_POSITION)
         - AccountRouter           (RT_REQ_ACCOUNT_SUMMURY)
         - Extend MarketDataRouter (RT_TICK_SIZE, RT_TICK_BY_TICK_DATA)
         Each router added one at a time, with dual-path verification
         ↓
Phase C: Migrate legacy strategies to LEGO blocks
         - Use AlphaModelAdapter, RiskModelAdapter, ExecutionModelAdapter
           (already built in Phase 3) to wrap existing CBasicAlphaModel, etc.
         - Migrate one strategy at a time, validate equivalence
         ↓
Phase D: Remove CDispatcher
         - Only when ZERO subscribers remain on CDispatcher
         - Remove CSubscriber, CDispatcher, GlobalReqManager
         - Remove CProcessingBase_v2::MessageHandler
         - Remove void* casting throughout
         - Clean up CBrokerDataProvider to only use typed routers
```

### What makes Phase D safe:

Each prior phase is **reversible**. If a typed router has a bug, the CDispatcher path still works as fallback. Only when every message type is confirmed working through the new typed path do we cut the CDispatcher wires.

### Subscriber audit (what needs migrating before CDispatcher removal):

| Class | Inherits CSubscriber via | Uses These Message Types |
|---|---|---|
| `CBasicRoot` | `CBaseModel → CProcessingBase_v2` | RT_NEXT_VALID_ID |
| `CBasicAccount` | `CBaseModel` | RT_REQ_ACCOUNT_SUMMURY |
| `CBasicPortfolio` | `CBaseModel` | RT_REQ_POSITION |
| `CBasicStrategy_V2` | `CBaseModel` | RT_TICK_PRICE, RT_REALTIME_BAR |
| `CBasicAlphaModel` | `CBaseModel` | RT_HISTORICAL_DATA |
| `CBasicRiskModel` | `CBaseModel` | RT_TICK_PRICE |
| `CBasicExecutionModel` | `CBaseModel` | RT_ORDER_EXECUTION, RT_ORDER_COMMISSION |
| `CBasicSelectionModel` | `CBaseModel` | RT_TICK_PRICE |
| `CBaseRebalanceModel` | `CBaseModel` | RT_TICK_PRICE |
| `AutoDeltAlignmentProcessing` | `CBaseModel` | RT_HISTORICAL_DATA, RT_REQ_OPTION_PRICE, RT_TICK_PRICE, RT_REQ_POSITION |
| `PairTraderPM` | Direct `CSubscriber` | RT_TICK_PRICE, RT_HISTORICAL_DATA |
| `CPresenter` | `CSubscriber` | RT_NEXT_VALID_ID, connection status |

---

## Updated Execution Order

```
Steps 1-3:  Activate data flow (MarketDataRouter receives live ticks+bars)
Steps 4-5:  Activate execution + supervision
Step 6:     PipelineFactory (create strategies from config)
Step 7:     Dual-path coexistence verified
Step 8:     Default LEGO models prove full integration path
            ─── MILESTONE: New pipeline runs end-to-end alongside legacy ───
Phase B:    Add remaining typed routers (one per message type)
Phase C:    Migrate legacy strategies (one at a time)
Phase D:    Remove CDispatcher (only when zero subscribers remain)
```

---

## Success Criteria

- [ ] `IBComClientImpl::tickPrice()` delivers ticks to both CDispatcher AND MarketDataRouter
- [ ] `IBComClientImpl::realtimeBar()` emits `barClose` on MarketDataRouter
- [ ] `MarketOrderExecutionBlock` places real orders through `IBOrderExecutionAdapter`
- [ ] `Supervisor` manages at least one `StrategyRuntime` in the running app
- [ ] Legacy strategies continue working unchanged (dual-path)
- [ ] A LEGO pipeline strategy can be created from JSON config and run end-to-end
- [ ] Default LEGO models (SimpleMomentum, DualAlpha) run successfully with simulated data
- [ ] StructuredLogger shows full correlation ID trace from tick → signal → target → execution
- [ ] All existing 224 tests still pass
- [ ] CDispatcher subscriber count can be monitored (preparation for Phase D audit)
