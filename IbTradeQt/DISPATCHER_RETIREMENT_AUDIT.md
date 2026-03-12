# CDispatcher Retirement Audit

**Date**: 2026-03-04
**Purpose**: Track migration status of every `CDispatcher`/`CSubscriber` message type to typed Qt signal/slot routers.

## Message Type Migration Status

| Message Type | Description | Typed Router | CDispatcher Forwarding | Status |
|---|---|---|---|---|
| `RT_NEXT_VALID_ID` | Next valid order ID | `OrderRouter::nextValidIdReceived` | **REMOVED** | **Phase B** — Legacy models receive via `CProcessingBase_v2::slotRouterNextValidId()` |
| `RT_REQ_ACCOUNT_SUMMURY` | Account summary data | `AccountRouter::accountSummaryUpdated` | **REMOVED** | **Phase B** — Adapter slot converts `AccountSummaryData` -> `CAccountSummary` |
| `RT_REQ_POSITION` | Position updates | `PositionRouter::positionChanged` / `positionSnapshotComplete` | **REMOVED** | **Phase B** — Adapter slots populate `m_positionMap`, emit `signalEndRecvPosition` |
| `RT_HISTORICAL_DATA` | Historical bar data | `HistoricalDataRouter::barsReceived` | **REMOVED** | **Phase B** — Adapter slot converts `HistoricalBar` -> `CHistoricalData`, emits `signalCbkRecvHistoricalData`. `CBrokerDataProvider::reqestHistoricalData()` calls `setReqIdSymbol()` |
| `RT_TICK_PRICE` | Tick price updates | `MarketDataRouter::tick` | Active (dual-path) | **Phase A** — Both CDispatcher and MarketDataRouter forward |
| `RT_TICK_SIZE` | Tick size updates | `MarketDataRouter::tickSizeUpdate` | Active (dual-path) | **Phase A** — Both CDispatcher and MarketDataRouter forward |
| `RT_REALTIME_BAR` | 5-second real-time bars | `MarketDataRouter::barClose` | Active (dual-path) | **Phase A** — Both CDispatcher and MarketDataRouter forward |
| `RT_TICK_BY_TICK_DATA` | Tick-by-tick trade data | `MarketDataRouter::tickByTickTrade` | Active (dual-path) | **Phase A** — Both CDispatcher and MarketDataRouter forward |
| `RT_ORDER_STATUS` | Order status changes | `OrderRouter::orderStatusChanged` | Active (dual-path) | **Phase A** — Both CDispatcher and OrderRouter forward |
| `RT_ORDER_EXECUTION` | Execution details | `OrderRouter::executionReceived` | Active (dual-path) | **Phase A** — Both CDispatcher and OrderRouter forward |
| `RT_ORDER_COMMISSION` | Commission reports | `OrderRouter::commissionReceived` | Active (dual-path) | **Phase A** — Both CDispatcher and OrderRouter forward |
| `RT_TICK_GENERIC` | Generic tick data | — | Active | **NOT MIGRATED** — Not used by pipeline blocks |
| `RT_TICK_STRING` | Tick string data | — | Active | **NOT MIGRATED** — Not used by pipeline blocks |
| `RT_HISTORICAL_TICK_DATA` | Historical tick data | — | Active | **NOT MIGRATED** — Low priority |
| `RT_MKT_DEPTH` | Market depth L1 | — | Active | **NOT MIGRATED** — Low priority |
| `RT_MKT_DEPTH_L2` | Market depth L2 | — | Active | **NOT MIGRATED** — Low priority |
| `RT_REQ_OPTION_PRICE` | Option price calc | — | Active | **NOT MIGRATED** — Options-specific |
| `RT_REQ_RESTART_SUBSCRIPTION` | Subscription restart | — | Active | **NOT MIGRATED** — Infrastructure concern |
| `RT_REQ_ERROR_SUBSRIPTION` | Error notification | — | Active | **NOT MIGRATED** — Infrastructure concern |
| `RT_REQ_ORDER_STATUS` | Order status subscription | — | Active | **REPLACED** by `OrderRouter` (direct forwarding) |
| `RT_REQ_CUR_TIME` | Current time | — | Active | **NOT MIGRATED** — Low priority |
| `RT_REQ_REL_DATA` | Real-time data subscription | — | Active | **REPLACED** by `MarketDataRouter` (direct forwarding) |

## CDispatcher Subscribers

| Subscriber Class | Messages Consumed | Phase B Status |
|---|---|---|
| `CProcessingBase_v2` | All `RT_*` types (main message handler) | **MIGRATED** for 4 types — `m_useTypedRouters` guard skips `RT_NEXT_VALID_ID`, `RT_REQ_ACCOUNT_SUMMURY`, `RT_REQ_POSITION`, `RT_HISTORICAL_DATA` when routers are connected |
| `CBasicRoot` | `RT_NEXT_VALID_ID` (broadcast) | **MIGRATED** — Receives via `OrderRouter::nextValidIdReceived` -> adapter slot |
| `CBasicAccount` | `RT_REQ_ACCOUNT_SUMMURY`, `RT_REQ_POSITION` | **MIGRATED** — Receives via `AccountRouter` and `PositionRouter` adapter slots |
| `CBasicAlphaModel` | `RT_HISTORICAL_DATA` | **MIGRATED** — Receives via `HistoricalDataRouter::barsReceived` adapter slot |
| `CBasicPortfolio` | Inherits from `CProcessingBase_v2` but does not directly subscribe | No migration needed |
| `AlphaModGetTime` | Timer-based only | No dispatcher dependency |
| `CBrokerDataProvider` | Subscription management | Carries typed router pointers since Phase B |

## Phase B Implementation Details

### Router Transport via CBrokerDataProvider
Typed router pointers (`OrderRouter*`, `AccountRouter*`, `PositionRouter*`, `HistoricalDataRouter*`) are stored in `CBrokerDataProvider` and propagated to all models via `setBrokerDataProvider()`.

### Adapter Slots in CProcessingBase_v2
When `setIBrokerDataProvider()` is called, `connectToTypedRouters()` connects router signals to adapter slots that convert Q_GADGET structs to legacy CObject types:
- `slotRouterNextValidId(int)` -> `setNextValidId()`
- `slotRouterAccountSummary(AccountSummaryData)` -> `CAccountSummary` -> `signalRecvAccountSummary`
- `slotRouterPositionChanged(PositionUpdate)` -> `CPosition` -> `m_positionMap`
- `slotRouterPositionSnapshotComplete()` -> `signalEndRecvPosition`
- `slotRouterBarsReceived(int, QString, QVector<HistoricalBar>)` -> `QList<CHistoricalData>` -> `signalCbkRecvHistoricalData`

### Historical Data reqId Mapping
`CBrokerDataProvider::reqestHistoricalData()` now calls `HistoricalDataRouter::setReqIdSymbol()` so the router knows which symbol each reqId maps to. This ensures `barsReceived()` emits with the correct symbol.

### m_useTypedRouters Guard
A flag in `CProcessingBase_v2` is set to `true` when router connections are established. `MessageHandler()` skips the 4 migrated message types when the flag is true, preventing duplicate processing.

## Retirement Roadmap

### Phase A: Dual-Forward (Complete)
All typed routers created and wired. `IBComClientImpl` forwards to both CDispatcher and typed routers.

### Phase B: Legacy Subscriber Migration (Complete)
4 message types fully migrated: `RT_NEXT_VALID_ID`, `RT_REQ_ACCOUNT_SUMMURY`, `RT_REQ_POSITION`, `RT_HISTORICAL_DATA`. CDispatcher forwarding removed for these types. Legacy models receive data through typed router adapter slots.

### Phase C: Migrate All Strategies to LEGO Blocks
- `cMomentum` -> `MomentumAlphaBlock` (migration config: `legacy_momentum_pipeline.json`)
- `CMovingAverageCrossover` -> `MovingAverageCrossoverAlphaBlock` (migration config: `ma_crossover_pipeline.json`)
- `csma` -> Requires new SMA alpha block
- `cteststrategy` -> Test adapter

### Phase D: Migrate Remaining CDispatcher Message Types
Remove CDispatcher forwarding for remaining dual-path types:
- `RT_TICK_PRICE`, `RT_TICK_SIZE`, `RT_REALTIME_BAR`, `RT_TICK_BY_TICK_DATA`
- `RT_ORDER_STATUS`, `RT_ORDER_EXECUTION`, `RT_ORDER_COMMISSION`
- Low-priority types on demand

### Phase E: Remove CDispatcher
Once all subscribers are migrated, remove:
- `CDispatcher` class and `Dispatcher.h/.cpp`
- `CSubscriber` base class
- `CProcessingBase_v2::MessageHandler()` callback
- Remaining `SendMessageToSubscribers()` calls from `IBComClientImpl`
- `CBrokerDataProvider` subscription management can be simplified
