# Close All Remaining Gaps — Refined Implementation Plan

**Created**: 2026-03-04
**Status**: PLAN — awaiting approval
**Goal**: Close all 8 gaps from `REMAINING_GAPS.md`, retire `CDispatcher`.

---

## Design Decisions (addressing plan weaknesses)

1. **No Phase 3 / Phase 4 overlap on positions.**
   Build `PositionRouter` once (Phase 3). `IBPositionRepositoryAdapter` consumes `PositionRouter` directly. No throwaway `PositionEventBridge` for positions.

2. **Historical data uses a pull-based `IHistoricalDataProvider` port.**
   Alpha blocks that need history call `provider->requestHistoricalData(...)` which returns a `QFuture<QVector<HistoricalBar>>`. Under the hood this wraps `IBComClientImpl::reqHistoricalDataAPI` and listens for `HistoricalDataRouter::barsReceived`. This keeps alpha blocks simple and testable.

3. **`BlockGraph` already has `selectionBlocks` vector** (line 24 of `StrategyPipelineRunner.h`), and `runSelection()` already calls them (line 136). `PipelineFactory::buildGraph` just needs to parse the new `"selection"` key from JSON.

4. **Limit/stop orders** require a new overload `reqPlaceOrderExAPI` on `IBrokerAPI` (not modifying the existing one, for backward compatibility). `IBOrderExecutionAdapter::placeOrder` inspects `intent.orderType` and `intent.limitPrice`.

5. **DB threading**: `SqlitePositionRepository` needs its own `QSqlDatabase` connection created on the calling thread. Since pipeline strategies run on their own `QThread`, the repository must be created in the main thread and all writes marshalled. We solve this by making `SqlitePositionRepository` a `QObject` with slots and using `Qt::QueuedConnection`.

6. **Legacy migration** focuses on `cMomentum` (simple: timer-based, no IB data dependencies) and `CMovingAverageCrossover` (needs positions + historical data). Both are mapped step-by-step.

---

## Phase 1: Quick Wins (Gap 8, Gap 3, Gap 4)

### Step 1.1: Wire `SqlitePositionRepository` (Gap 8)

**Problem**: `CApplicationController` uses `MockPositionRepository`. Position state lost on restart.

**Existing DB table**: `Positions` table already created by `DBHandler::initializeDatabase()` in `DB/dbhandler.cpp:64` with schema:
```sql
CREATE TABLE IF NOT EXISTS Positions (
    strategyId VARCHAR(64),
    symbol VARCHAR(10),
    quantity INT DEFAULT 0,
    averageOpenPrice DOUBLE DEFAULT 0,
    pnl DOUBLE DEFAULT 0, fee DOUBLE DEFAULT 0,
    openDate TEXT, closeDate TEXT, status INT,
    PRIMARY KEY (strategyId, symbol)
);
```

**Schema match with `SqlitePositionRepository`**: The adapter reads columns `strategyId, symbol, quantity, averageOpenPrice` — these all exist. The `ON CONFLICT(strategyId, symbol)` in the adapter's `updatePosition` matches the `PRIMARY KEY (strategyId, symbol)`. No schema changes needed.

**Threading solution**: `SqlitePositionRepository` creates its own `QSqlDatabase` connection (separate from `DBHandler`) using a unique connection name. It runs in the **main thread** (same as `CApplicationController`). Pipeline threads call `getCurrentPositions()` which calls `IPositionRepositoryPort::getAllPositions()`. Since `SqlitePositionRepository` is NOT a `QObject`, calls from pipeline threads go directly to it. SQLite supports concurrent reads from multiple threads with WAL mode. We add `PRAGMA journal_mode=WAL` on connection open.

**Changes**:

| File | Change |
|------|--------|
| `Adapters/SqlitePositionRepository.h` | Add WAL pragma in constructor after opening connection. Add explicit `init()` method that opens DB and sets WAL. |
| `MainSystem/capplicationcontroller.h` | Replace `MockPositionRepository m_positionRepo` → `SqlitePositionRepository* m_pPositionRepo = nullptr;` |
| `MainSystem/capplicationcontroller.cpp` | Create `SqlitePositionRepository` with a new unique connection name, pointing to `myLocalDb.sqlite`. Call `CPipelineStrategyAdapter::setGlobalPositionRepo(m_pPositionRepo)`. Delete in destructor. |
| `ibtrading.pro` | Ensure `QT += sql` is present (it already is for `DBHandler`). |

**Implementation detail** — `SqlitePositionRepository` constructor change:
```cpp
explicit SqlitePositionRepository(const QString& dbPath, QObject* parent = nullptr)
    : m_connectionName("pipeline_positions_" + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);
    db.open();
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
}
```

**Test**: New test `tst_sqlite_position_repo` — insert position, restart repo, verify persistence.

---

### Step 1.2: Create `StaticListSelectionBlock` (Gap 3)

**Problem**: Only `PassAllSelectionBlock` exists. No way to filter universe from config.

**Changes**:

| File | Change |
|------|--------|
| `Blocks/StaticListSelectionBlock.h` | **NEW** — Reads `"symbols": ["AAPL","MSFT",...]` from config. `select()` returns intersection of universe and config list. |
| `Pipeline/PipelineFactory.h` | Add `createSelectionBlock(blockId)` method. Parse `"selection"` key from config JSON. Wire into `graph.selectionBlocks`. |
| `ibtrading.pro` | Add new header. |

**`PipelineFactory::buildGraph` addition** (after alpha parsing, before rebalance):
```cpp
QJsonArray selectionConfigs = config.value("selection").toArray();
for (const auto& selVal : selectionConfigs) {
    QJsonObject selCfg = selVal.toObject();
    QString blockId = selCfg.value("blockId").toString();
    ISelectionBlock* sel = createSelectionBlock(blockId);
    if (sel) {
        sel->setConfig(selCfg.value("config").toObject());
        graph.selectionBlocks.append(sel);
    }
}
```

**`createSelectionBlock`**:
```cpp
static ISelectionBlock* createSelectionBlock(const QString& blockId) {
    if (blockId == "pass-all") return new Blocks::PassAllSelectionBlock();
    if (blockId == "static-list") return new Blocks::StaticListSelectionBlock();
    auto result = BlockRegistry::instance().createBlock(blockId);
    if (result) return qobject_cast<ISelectionBlock*>(*result);
    return new Blocks::PassAllSelectionBlock();
}
```

**`StaticListSelectionBlock::select`**:
```cpp
QVector<QString> select(const QVector<QString>& universe) override {
    if (m_symbols.isEmpty()) return universe;
    QVector<QString> result;
    for (const auto& sym : universe) {
        if (m_symbols.contains(sym)) result.append(sym);
    }
    return result.isEmpty() ? m_symbols : result; // if no universe, return configured symbols
}
```

**Test**: Unit test confirming intersection logic and empty universe fallback.

---

### Step 1.3: Pipeline Config Picker UI (Gap 4)

**Problem**: `slotOnClickAddPipelineStrategy()` hardcodes `simple_momentum_pipeline.json`.

**Changes**:

| File | Change |
|------|--------|
| `MainSystem/CPortfolioConfigModel.h` | Add `void slotOnClickAddPipelineStrategyWithConfig(const QString& configPath);` |
| `MainSystem/portfolioconfigmodel.cpp` | New slot scans `Strategies/DefaultPipelines/` for `*.json` files. `slotOnClickAddPipelineStrategy()` now builds a `QMenu` submenu dynamically listing each JSON config by its `"name"` field. Each action connects to `slotOnClickAddPipelineStrategyWithConfig(path)`. |
| `MainSystem/portfolioconfigmodel.cpp` line 289 | `addModel` `PM_ITEM_PIPELINE_STRATEGY` case receives `configPath` via a member variable or overloaded addModel. |

**UI flow**:
1. User right-clicks portfolio node
2. Menu shows "Add Pipeline Strategy >" as a submenu
3. Submenu lists: "Simple Momentum Strategy", "Dual Alpha Strategy" (read from JSON `name` field)
4. Clicking one calls `slotOnClickAddPipelineStrategyWithConfig("Strategies/DefaultPipelines/dual_alpha_pipeline.json")`

**Implementation approach**: The right-click context menu is built in a method that creates `QAction`s. We change the single "Add Pipeline Strategy (LEGO)" action to a `QMenu` with sub-actions. If only one config exists, it acts as a simple action. The selected config path is stored in a member variable before calling `addModel`.

**Where the context menu is built**: Need to find this — likely in `CPortfolioConfigModel` or `CPresenter` or the view. The slot `slotOnClickAddPipelineStrategy` is connected somewhere.

**Test**: Manual test (verify submenu appears, verify correct config loads). No automated test needed.

---

## Phase 2: Limit/Stop Order Support (Gap 5)

### Step 2.1: Extend `IBrokerAPI` with typed order placement

**Problem**: `reqPlaceOrderAPI(symbol, qty, action)` always places market orders (line 230: `orderToPlace.order.orderType = "MKT"`). `ExecutionIntent` has `Limit` and `Stop` enums but the adapter ignores them.

**Changes**:

| File | Change |
|------|--------|
| `IBComm/IBrokerAPI.h` | Add new virtual: `virtual qint32 reqPlaceOrderExAPI(const QString& symbol, qint32 quantity, eOrderAction_t action, const QString& orderType, double limitPrice = 0.0, double auxPrice = 0.0) { return reqPlaceOrderAPI(symbol, quantity, action); }` — default delegates to old method for backward compatibility. |
| `IBComm/IBComClientIpml.cpp` | Implement `reqPlaceOrderExAPI` override: copies from `reqPlaceOrderAPI` but sets `orderToPlace.order.orderType` from the `orderType` parameter, `orderToPlace.order.lmtPrice = limitPrice`, `orderToPlace.order.auxPrice = auxPrice`. |
| `IBComm/IBComClientImpl.h` | Declare the override. |
| `Adapters/IBOrderExecutionAdapter.h` | In `placeOrder()`, map `intent.orderType` to IB order type string: `Market→"MKT"`, `Limit→"LMT"`, `Stop→"STP"`. Extract `intent.limitPrice` (for LMT: `limitPrice`, for STP: `auxPrice`). Call `reqPlaceOrderExAPI` instead of `reqPlaceOrderAPI`. |
| `Blocks/LimitOrderExecutionBlock.h` | **NEW** — Like `MarketOrderExecutionBlock` but sets `intent.orderType = Limit` and `intent.limitPrice` from its config `"limitOffset"` (e.g., mid - offset). |
| `Pipeline/PipelineFactory.h` | Register `"limit-order-execution"` in `createExecutionBlock`. |
| `ibtrading.pro` | Add new headers. |

**IB Order field mapping**:
| Pipeline `OrderType` | IB `orderType` | IB `lmtPrice` | IB `auxPrice` |
|---|---|---|---|
| `Market` | `"MKT"` | 0 | 0 |
| `Limit` | `"LMT"` | `intent.limitPrice.value()` | 0 |
| `Stop` | `"STP"` | 0 | `intent.limitPrice.value()` |

**`IBComClientImpl::reqPlaceOrderExAPI` implementation**:
```cpp
qint32 IBComClientImpl::reqPlaceOrderExAPI(
    const QString& _symbol, const qint32 _quantity,
    const eOrderAction_t _action, const QString& _orderType,
    double _limitPrice, double _auxPrice)
{
    reqPlaceOrder_t orderToPlace;
    qint32 retOrderId = getNexValidId();

    orderToPlace.contract.symbol = _symbol.toLocal8Bit().data();
    orderToPlace.contract.secType = "STK";
    orderToPlace.contract.exchange = "SMART";

    orderToPlace.order.action = (OA_BUY == _action) ? "BUY" : "SELL";
    orderToPlace.order.orderType = _orderType.toStdString();
    orderToPlace.order.tif = "DAY";
    orderToPlace.order.totalQuantity = DecimalFunctions::doubleToDecimal(_quantity);
    orderToPlace.order.transmit = true;
    orderToPlace.order.orderId = retOrderId;

    if (_orderType == "LMT") orderToPlace.order.lmtPrice = _limitPrice;
    if (_orderType == "STP") orderToPlace.order.auxPrice = _auxPrice;

    m_pClient->placeOrder(retOrderId, orderToPlace.contract, orderToPlace.order);
    return retOrderId;
}
```

**Test**: Extend `tst_live_execution_wiring.h` with `MockBrokerAPI` that records order type/price; verify limit and stop orders are placed correctly.

---

## Phase 3: Live Positions + Position Router (Gap 2 + part of Gap 1)

### Step 3.1: Create `PositionRouter`

**Problem**: `position()` callback only goes through `CDispatcher`.

**New file**: `IBComm/PositionRouter.h`

```cpp
namespace IBComm {

struct PositionUpdate {
    Q_GADGET
    Q_PROPERTY(QString account MEMBER account)
    Q_PROPERTY(QString symbol MEMBER symbol)
    Q_PROPERTY(double quantity MEMBER quantity)
    Q_PROPERTY(double avgCost MEMBER avgCost)
public:
    QString account;
    QString symbol;
    double quantity = 0.0;
    double avgCost = 0.0;
};

class PositionRouter : public QObject {
    Q_OBJECT
public:
    explicit PositionRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onPosition(const QString& account, const QString& symbol,
                    double position, double avgCost) {
        PositionUpdate update{account, symbol, position, avgCost};
        m_positions[symbol] = update;
        emit positionChanged(update);
    }

    void onPositionEnd() {
        emit positionSnapshotComplete();
    }

signals:
    void positionChanged(const IBComm::PositionUpdate& update);
    void positionSnapshotComplete();

private:
    QMap<QString, PositionUpdate> m_positions;
};

} // namespace IBComm
Q_DECLARE_METATYPE(IBComm::PositionUpdate)
```

### Step 3.2: Create `IBPositionRepositoryAdapter`

**New file**: `Adapters/IBPositionRepositoryAdapter.h`

```cpp
class IBPositionRepositoryAdapter : public QObject, public Ports::IPositionRepositoryPort {
    Q_OBJECT
public:
    explicit IBPositionRepositoryAdapter(QObject* parent = nullptr) : QObject(parent) {}

    void connectToRouter(IBComm::PositionRouter* router) {
        connect(router, &IBComm::PositionRouter::positionChanged,
                this, &IBPositionRepositoryAdapter::onPositionChanged,
                Qt::QueuedConnection);
    }

    // IPositionRepositoryPort overrides
    Expected<Ports::PositionRow, Error> getPosition(int strategyId, const QString& symbol) override {
        Q_UNUSED(strategyId) // IB positions are account-wide
        QMutexLocker lock(&m_mutex);
        auto it = m_positions.find(symbol);
        if (it == m_positions.end())
            return make_unexpected(Error{ErrorCode::NotFound, "Position not found", "IBPositionRepositoryAdapter"});
        return it.value();
    }

    Expected<QVector<Ports::PositionRow>, Error> getAllPositions(int strategyId) override {
        Q_UNUSED(strategyId)
        QMutexLocker lock(&m_mutex);
        QVector<Ports::PositionRow> result;
        for (const auto& row : m_positions) result.push_back(row);
        return result;
    }

    Expected<void, Error> updatePosition(const Ports::PositionRow& position) override {
        QMutexLocker lock(&m_mutex);
        m_positions[position.symbol] = position;
        return {};
    }

private slots:
    void onPositionChanged(const IBComm::PositionUpdate& update) {
        Ports::PositionRow row;
        row.symbol = update.symbol;
        row.quantity = update.quantity;
        row.avgCost = update.avgCost;
        row.account = update.account;
        QMutexLocker lock(&m_mutex);
        m_positions[update.symbol] = row;
    }

private:
    QMap<QString, Ports::PositionRow> m_positions;
    QMutex m_mutex;
};
```

### Step 3.3: Wire in `IBComClientImpl` and `CApplicationController`

| File | Change |
|------|--------|
| `IBComm/IBComClientImpl.h` | Add `void setPositionRouter(IBComm::PositionRouter*);` and `IBComm::PositionRouter* m_positionRouter = nullptr;` |
| `IBComm/IBComClientIpml.cpp` `position()` | After existing `CDispatcher` send, forward to `m_positionRouter->onPosition(account, symbol, pos, avgCost)`. Same for `positionEnd()` → `m_positionRouter->onPositionEnd()`. |
| `MainSystem/capplicationcontroller.h` | Add `IBComm::PositionRouter* m_pPositionRouter = nullptr;` and `IBPositionRepositoryAdapter* m_pLivePositionRepo = nullptr;` |
| `MainSystem/capplicationcontroller.cpp` | Create `PositionRouter`, `IBPositionRepositoryAdapter`, connect them. Wire `implClient->setPositionRouter(m_pPositionRouter)`. Set `CPipelineStrategyAdapter::setGlobalPositionRepo(m_pLivePositionRepo)` (replacing `SqlitePositionRepository` from Phase 1 — now live positions use IB data, persistent positions use SQLite — see design note below). |
| `ibtrading.pro` | Add new headers. |

**Design note on dual position repos**: Two repositories serve different needs:
- **`IBPositionRepositoryAdapter`**: Live positions from IB (account-wide, reflects broker state). Used as the **global position repo** for pipeline strategies in Live mode.
- **`SqlitePositionRepository`**: Persistent position tracking per strategy. Used in DryRun mode and for audit trail.

`CPipelineStrategyAdapter::start()` already picks repo based on execution mode:
```cpp
auto* posRepo = (m_execMode == ExecutionMode::Live && s_globalPositionRepo)
    ? s_globalPositionRepo : static_cast<Ports::IPositionRepositoryPort*>(&m_mockPositionRepo);
```

We change this to use `SqlitePositionRepository` instead of `MockPositionRepository` as the DryRun fallback:
```cpp
auto* posRepo = (m_execMode == ExecutionMode::Live && s_globalPositionRepo)
    ? s_globalPositionRepo : s_globalPersistentPositionRepo;
```

Add a new static `s_globalPersistentPositionRepo` for SQLite.

**Test**: `tst_position_router` — feed `onPosition()`, verify `IBPositionRepositoryAdapter::getAllPositions()` returns correct data. Test thread safety with concurrent reads.

---

## Phase 4: Typed Routers for Remaining Message Types (Gap 1 + Gap 7)

### Step 4.1: `HistoricalDataRouter` + `IHistoricalDataProvider` (Gap 7)

**Problem**: Historical data is request/response, not push. Blocks need to pull historical bars.

**New port**: `Ports/IHistoricalDataProvider.h`
```cpp
namespace Ports {

struct HistoricalBar {
    Q_GADGET
public:
    QString symbol;
    QDateTime timestamp;
    double open = 0.0, high = 0.0, low = 0.0, close = 0.0;
    double volume = 0.0;
    int count = 0;
};

class IHistoricalDataProvider {
public:
    virtual ~IHistoricalDataProvider() = default;

    struct HistRequest {
        QString symbol;
        QString duration;   // e.g. "10 D"
        QString barSize;    // e.g. "1 day"
    };

    virtual void requestHistoricalData(
        int requestId,
        const HistRequest& request) = 0;

signals: // conceptual — implemented via QObject in concrete class
    // void barsReceived(int requestId, const QVector<Ports::HistoricalBar>& bars);
};

} // namespace Ports
Q_DECLARE_METATYPE(Ports::HistoricalBar)
```

**New router**: `IBComm/HistoricalDataRouter.h`
```cpp
namespace IBComm {

class HistoricalDataRouter : public QObject {
    Q_OBJECT
public:
    explicit HistoricalDataRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onHistoricalBar(int reqId, const QString& dateStr,
                         double open, double high, double low, double close,
                         double volume, int count) {
        Ports::HistoricalBar bar;
        bar.timestamp = QDateTime::fromString(dateStr, Qt::ISODate);
        bar.open = open; bar.high = high; bar.low = low; bar.close = close;
        bar.volume = volume; bar.count = count;
        m_pendingBars[reqId].append(bar);
    }

    void onHistoricalDataEnd(int reqId) {
        auto bars = m_pendingBars.take(reqId);
        emit barsReceived(reqId, bars);
    }

signals:
    void barsReceived(int requestId, const QVector<Ports::HistoricalBar>& bars);

private:
    QMap<int, QVector<Ports::HistoricalBar>> m_pendingBars;
};

} // namespace IBComm
```

**New adapter**: `Adapters/IBHistoricalDataAdapter.h` — Implements `IHistoricalDataProvider`, holds `IBrokerAPI*`, calls `reqHistoricalDataAPI`. Connects to `HistoricalDataRouter::barsReceived` to relay results.

```cpp
class IBHistoricalDataAdapter : public QObject, public Ports::IHistoricalDataProvider {
    Q_OBJECT
public:
    IBHistoricalDataAdapter(IBrokerAPI* broker, IBComm::HistoricalDataRouter* router, QObject* parent = nullptr)
        : QObject(parent), m_broker(broker), m_router(router)
    {
        connect(m_router, &IBComm::HistoricalDataRouter::barsReceived,
                this, &IBHistoricalDataAdapter::onBarsReceived);
    }

    void requestHistoricalData(int requestId, const HistRequest& request) override {
        m_requestSymbols[requestId] = request.symbol;
        reqHistConfigData_t config(requestId, request.barSize, request.duration, request.symbol);
        m_broker->reqHistoricalDataAPI(config);
    }

signals:
    void barsReceived(int requestId, const QString& symbol, const QVector<Ports::HistoricalBar>& bars);

private slots:
    void onBarsReceived(int requestId, const QVector<Ports::HistoricalBar>& bars) {
        QString symbol = m_requestSymbols.take(requestId);
        emit barsReceived(requestId, symbol, bars);
    }

private:
    IBrokerAPI* m_broker;
    IBComm::HistoricalDataRouter* m_router;
    QMap<int, QString> m_requestSymbols;
};
```

**Integration into `IAlphaBlock`**: Add an optional setter:
```cpp
// In IAlphaBlock.h
virtual void setHistoricalDataProvider(Ports::IHistoricalDataProvider* /*provider*/) {}
```
Alpha blocks that need history override this and store the provider. They call `provider->requestHistoricalData(...)` in `initialize()` and process bars asynchronously.

**Wire in `IBComClientIpml.cpp`**:
- `historicalData()` — after `CDispatcher` send, forward to `m_historicalDataRouter->onHistoricalBar(reqId, bar.time, bar.open, ...)`.
- `historicalDataEnd()` — forward to `m_historicalDataRouter->onHistoricalDataEnd(reqId)`.

**Wire in `CApplicationController`**: Create `HistoricalDataRouter*`, `IBHistoricalDataAdapter*`, set on `IBComClientImpl`, make available to pipeline via new static on `CPipelineStrategyAdapter`.

**Wire in `PipelineFactory::createRuntime`**: After building graph, if `IHistoricalDataProvider*` is available, call `alpha->setHistoricalDataProvider(provider)` for each alpha block.

**Test**: `tst_historical_data_router` — feed bars + end signal, verify `barsReceived` emission. Mock-based `IHistoricalDataProvider` for alpha block tests.

---

### Step 4.2: Extend `MarketDataRouter` for tick-by-tick (Gap 7 continued)

**Changes**:

| File | Change |
|------|--------|
| `IBComm/MarketDataRouter.h` | Add `TickByTickTrade` struct (Q_GADGET: `symbol`, `price`, `size`, `timestamp`, `exchange`). Add slot `onTickByTick(int reqId, ...)`. Add signal `tickByTickTrade(const TickByTickTrade&)`. |
| `IBComm/IBComClientIpml.cpp` `tickByTickAllLast()` | After `CDispatcher` send, forward to `m_marketDataRouter->onTickByTick(reqId, symbol, price, size, timestamp, exchange)`. |
| `Pipeline/IAlphaBlock.h` | Add optional slot: `virtual void onTickByTick(const IBComm::TickByTickTrade&) {}` |
| `Supervision/StrategyRuntime.h` `connectToMarketData()` | Also connect `router->tickByTickTrade` to alpha blocks' `onTickByTick`. |

**Test**: Unit test feeding tick-by-tick data through router, verifying alpha block receives it.

---

### Step 4.3: `OrderRouter` (replaces/extends `OrderEventBridge`)

**Problem**: `OrderEventBridge` is a simple relay. A proper `OrderRouter` provides typed signals for order status, executions, commissions, and next valid ID.

**New file**: `IBComm/OrderRouter.h`
```cpp
namespace IBComm {

struct OrderStatusUpdate {
    Q_GADGET
public:
    int orderId = 0;
    QString status;
    double filled = 0.0;
    double remaining = 0.0;
    double avgFillPrice = 0.0;
};

struct ExecutionReport {
    Q_GADGET
public:
    int orderId = 0;
    QString symbol;
    double avgPrice = 0.0;
    double shares = 0.0;
    QString execId;
};

struct CommissionReport {
    Q_GADGET
public:
    QString execId;
    double commission = 0.0;
    QString currency;
    double realizedPnL = 0.0;
};

class OrderRouter : public QObject {
    Q_OBJECT
public:
    explicit OrderRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onOrderStatus(int orderId, const QString& status, double filled,
                       double remaining, double avgFillPrice);
    void onExecDetails(int orderId, const QString& symbol, double avgPrice,
                       double shares, const QString& execId);
    void onCommissionReport(const QString& execId, double commission,
                            const QString& currency, double realizedPnL);
    void onNextValidId(int orderId);

signals:
    void orderStatusChanged(const IBComm::OrderStatusUpdate& update);
    void executionReceived(const IBComm::ExecutionReport& report);
    void commissionReceived(const IBComm::CommissionReport& report);
    void nextValidIdReceived(int orderId);
};

} // namespace IBComm
```

**Migration from `OrderEventBridge`**: `IBOrderExecutionAdapter` currently receives updates via `OrderEventBridge::onOrderStatus`. We rewire it to listen to `OrderRouter::orderStatusChanged` and `OrderRouter::executionReceived` instead. `OrderEventBridge` becomes deprecated (kept for one phase, then removed).

**Wire in `IBComClientIpml.cpp`**: Forward `orderStatus()`, `execDetails()`, `commissionReport()`, `nextValidId()` to `OrderRouter` (in addition to existing `CDispatcher` sends).

**Test**: Unit test for each signal path.

---

### Step 4.4: `AccountRouter`

**New file**: `IBComm/AccountRouter.h`
```cpp
namespace IBComm {

struct AccountSummaryData {
    Q_GADGET
public:
    QString account;
    QString accountType;
    double buyingPower = 0.0;
    double totalCashValue = 0.0;
    double netLiquidation = 0.0;
    double equityWithLoanValue = 0.0;
    QString currency;
};

class AccountRouter : public QObject {
    Q_OBJECT
public:
    explicit AccountRouter(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void onAccountSummary(const QString& account, const QString& tag,
                          const QString& value, const QString& currency);
    void onAccountSummaryEnd(int reqId);

signals:
    void accountSummaryUpdated(const IBComm::AccountSummaryData& summary);

private:
    AccountSummaryData m_current;
};

} // namespace IBComm
```

The slot accumulates tag/value pairs (same logic as `IBComClientImpl::accountSummary()` lines 501-520) and emits `accountSummaryUpdated` on `onAccountSummaryEnd`.

**Wire in `IBComClientIpml.cpp`**: Forward `accountSummary()` and `accountSummaryEnd()` to `AccountRouter`.

---

### Step 4.5: Wire all routers in `CApplicationController`

| Router | Created in | Wired to `IBComClientImpl` via |
|--------|-----------|-------------------------------|
| `MarketDataRouter` | Already exists in `CPresenter` | Already wired |
| `PositionRouter` | Phase 3 | `setPositionRouter()` |
| `HistoricalDataRouter` | Phase 4.1 | `setHistoricalDataRouter()` |
| `OrderRouter` | Phase 4.3 | `setOrderRouter()` |
| `AccountRouter` | Phase 4.4 | `setAccountRouter()` |

Each `set*Router` method stores a pointer. The callback methods in `IBComClientIpml.cpp` check for null before forwarding (same pattern as existing `m_marketDataRouter` check).

**`ibtrading.pro`**: Add all new headers.

**Test**: Integration test creating all routers, feeding IB-style callbacks, verifying typed signals.

---

## Phase 5: Legacy Strategy Migration (Gap 6)

### Step 5.1: Migrate `cMomentum`

**Analysis**: `cMomentum` is simple:
- No IB data subscriptions (no historical, no positions)
- Uses a 10-second timer to call `onTimeoutSlot` (currently just logs)
- Calls `createDataList()` (returns empty) → `emit dataProcessed`
- Depends on `CBasicStrategy_V2` → `CBaseModel` → `CProcessingBase_v2`

**Migration approach**: Create `MomentumAlphaBlock` already exists! It generates signals on `onTick()` based on price momentum. The legacy `cMomentum` is essentially a shell around a timer that doesn't do real alpha work.

**Action**:
1. Create a new default pipeline JSON `timer_momentum_pipeline.json` that uses `momentum-alpha` block with the same parameters as legacy `cMomentum`.
2. Verify the pipeline version produces equivalent behavior when fed the same tick stream.
3. Document that `cMomentum` can be retired when users migrate to the pipeline version.

**No code deletion yet** — just verification and documentation.

### Step 5.2: Migrate `CMovingAverageCrossover`

**Analysis**: More complex:
- Subscribes to `RT_REQ_POSITION` via `pbRequestPosition()`
- Has `slotCbkRecvHistoricalData` (currently ignores payload)
- Builds `m_assetList` from positions in `slotEndRecvPosition`
- Parameters: `Asset` (symbol), `MA_Fast` (5), `MA_Slow` (15)

**Migration approach**:
1. Create `MovingAverageCrossoverAlphaBlock` — an `IAlphaBlock` that:
   - Overrides `setHistoricalDataProvider` to store the provider
   - In `initialize()`, requests historical bars for the configured symbol
   - On `barsReceived`, computes fast and slow MAs
   - On each `onTick`, updates the latest bar, recomputes MAs, emits `signalGenerated` when crossover detected
2. Create `ma_crossover_pipeline.json` config referencing this alpha block
3. Run both legacy and pipeline side-by-side, compare outputs

**Dependencies**: Requires `IHistoricalDataProvider` from Phase 4.1.

**Test**: Unit test with mock historical data provider, feeding known OHLCV bars, verifying crossover detection.

### Step 5.3: Audit `CDispatcher` subscribers, document removal path

**Action**:
1. Grep all `Subscribe(` calls and `MessageHandler` implementations
2. List every consumer of each `tEReqType`
3. For each consumer, check if a typed router equivalent exists
4. Produce `DISPATCHER_RETIREMENT_AUDIT.md` with:
   - Per-message-type: old consumer → new router → migration status
   - Remaining blockers (e.g., `RT_REQ_OPTION_PRICE` for `AutoDeltAlignmentProcessing`)
   - Estimated effort to remove each `CDispatcher` dependency

**No code changes** — pure analysis document.

---

## Phase 6: Cleanup and Verification

### Step 6.1: Remove `OrderEventBridge` (replaced by `OrderRouter`)

- Delete `Adapters/OrderEventBridge.h`
- Update `IBComClientImpl.h` to remove `setOrderEventBridge` and use `setOrderRouter`
- Update `CApplicationController` to remove `m_pOrderEventBridge`
- Update `ibtrading.pro`

### Step 6.2: Update `REMAINING_GAPS.md` and `ARCHITECTURE.md`

- Mark closed gaps
- Document new routers, ports, and adapters
- Update the `CDispatcher` retirement roadmap

### Step 6.3: Full regression test run

- Run all 264+ existing tests
- Run all new tests from each phase
- Verify no regressions

---

## Summary: Files Created/Modified per Phase

| Phase | New Files | Modified Files |
|-------|-----------|----------------|
| 1.1 | `tst_sqlite_position_repo.h` | `SqlitePositionRepository.h`, `capplicationcontroller.h/.cpp` |
| 1.2 | `Blocks/StaticListSelectionBlock.h` | `PipelineFactory.h`, `ibtrading.pro` |
| 1.3 | — | `CPortfolioConfigModel.h`, `portfolioconfigmodel.cpp` |
| 2 | `Blocks/LimitOrderExecutionBlock.h` | `IBrokerAPI.h`, `IBComClientImpl.h`, `IBComClientIpml.cpp`, `IBOrderExecutionAdapter.h`, `PipelineFactory.h`, `ibtrading.pro` |
| 3 | `IBComm/PositionRouter.h`, `Adapters/IBPositionRepositoryAdapter.h` | `IBComClientImpl.h`, `IBComClientIpml.cpp`, `capplicationcontroller.h/.cpp`, `cpipelinestrategyadapter.h`, `ibtrading.pro` |
| 4.1 | `IBComm/HistoricalDataRouter.h`, `Ports/IHistoricalDataProvider.h`, `Adapters/IBHistoricalDataAdapter.h` | `IBComClientImpl.h`, `IBComClientIpml.cpp`, `IAlphaBlock.h`, `PipelineFactory.h`, `StrategyRuntime.h`, `cpipelinestrategyadapter.h`, `capplicationcontroller.h/.cpp`, `ibtrading.pro` |
| 4.2 | — | `MarketDataRouter.h`, `IBComClientIpml.cpp`, `IAlphaBlock.h`, `StrategyRuntime.h` |
| 4.3 | `IBComm/OrderRouter.h` | `IBComClientImpl.h`, `IBComClientIpml.cpp`, `IBOrderExecutionAdapter.h`, `capplicationcontroller.h/.cpp`, `ibtrading.pro` |
| 4.4 | `IBComm/AccountRouter.h` | `IBComClientImpl.h`, `IBComClientIpml.cpp`, `capplicationcontroller.h/.cpp`, `ibtrading.pro` |
| 5.1 | `Strategies/DefaultPipelines/timer_momentum_pipeline.json` | — |
| 5.2 | `Blocks/MovingAverageCrossoverAlphaBlock.h` | `PipelineFactory.h`, `ibtrading.pro` |
| 5.3 | `DISPATCHER_RETIREMENT_AUDIT.md` | — |
| 6.1 | — | Remove `OrderEventBridge.h`, `IBComClientImpl.h`, `capplicationcontroller.h/.cpp`, `ibtrading.pro` |
| 6.2 | — | `REMAINING_GAPS.md`, `ARCHITECTURE.md` |

---

## Dependency Graph

```
Phase 1.1 (SqlitePositionRepo) ──┐
Phase 1.2 (StaticListSelection)  ├── independent, can run in parallel
Phase 1.3 (Config Picker UI) ────┘

Phase 2 (Limit/Stop Orders) ──── independent

Phase 3 (PositionRouter + IBPositionRepo) ──── depends on Phase 1.1 (uses SQLite as DryRun fallback)

Phase 4.1 (HistoricalDataRouter) ──── independent
Phase 4.2 (Tick-by-tick) ──── independent
Phase 4.3 (OrderRouter) ──── independent
Phase 4.4 (AccountRouter) ──── independent
Phase 4.5 (Wire all) ──── depends on 4.1-4.4

Phase 5.1 (Migrate Momentum) ──── depends on Phase 1.2 (selection block in config)
Phase 5.2 (Migrate MA Crossover) ──── depends on Phase 4.1 (IHistoricalDataProvider)
Phase 5.3 (Dispatcher audit) ──── depends on Phase 4.5 + 5.1 + 5.2

Phase 6 (Cleanup) ──── depends on all above
```

---

## Estimated Effort

| Phase | Effort | Risk |
|-------|--------|------|
| 1.1 | Small | Low — table exists, adapter exists |
| 1.2 | Small | Low — interface exists, runner already calls it |
| 1.3 | Small | Low — UI change, scan directory |
| 2 | Medium | Medium — IB order types need testing with TWS |
| 3 | Medium | Medium — threading with QMutex |
| 4.1 | Medium | Medium — request/response async pattern |
| 4.2 | Small | Low — extends existing router |
| 4.3 | Medium | Low — follows existing pattern |
| 4.4 | Small | Low — simple accumulator |
| 4.5 | Small | Low — wiring only |
| 5.1 | Small | Low — block already exists |
| 5.2 | Medium | Medium — new alpha block with history |
| 5.3 | Small | Low — analysis only |
| 6 | Small | Low — cleanup |
