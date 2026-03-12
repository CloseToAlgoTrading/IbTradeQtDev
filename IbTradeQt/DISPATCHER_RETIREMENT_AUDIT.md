# CDispatcher Retirement Audit

**Date**: 2026-03-04
**Purpose**: Track migration status of every `CDispatcher`/`CSubscriber` message type to typed Qt signal/slot routers.

## Message Type Migration Status

| Message Type | Description | Typed Router | Status |
|---|---|---|---|
| `RT_TICK_PRICE` | Tick price updates | `MarketDataRouter::tick` | **MIGRATED** — `IBComClientImpl` forwards to `MarketDataRouter::onTickPrice()` |
| `RT_TICK_SIZE` | Tick size updates | `MarketDataRouter::tickSizeUpdate` | **MIGRATED** — `IBComClientImpl` forwards to `MarketDataRouter::onTickSize()` |
| `RT_REALTIME_BAR` | 5-second real-time bars | `MarketDataRouter::barClose` | **MIGRATED** — `IBComClientImpl` forwards to `MarketDataRouter::onBarComplete()` |
| `RT_TICK_BY_TICK_DATA` | Tick-by-tick trade data | `MarketDataRouter::tickByTickTrade` | **MIGRATED** — `IBComClientImpl` forwards to `MarketDataRouter::onTickByTickAllLast()` |
| `RT_HISTORICAL_DATA` | Historical bar data | `HistoricalDataRouter::historicalBar` / `barsReceived` | **MIGRATED** — `IBComClientImpl::historicalData()` and `historicalDataEnd()` forward to `HistoricalDataRouter` |
| `RT_REQ_POSITION` | Position updates | `PositionRouter::positionChanged` / `positionSnapshotComplete` | **MIGRATED** — `IBComClientImpl::position()` and `positionEnd()` forward to `PositionRouter` |
| `RT_ORDER_STATUS` | Order status changes | `OrderRouter::orderStatusChanged` | **MIGRATED** — `IBComClientImpl::orderStatus()` forwards to `OrderRouter` |
| `RT_ORDER_EXECUTION` | Execution details | `OrderRouter::executionReceived` | **MIGRATED** — `IBComClientImpl::execDetails()` forwards to `OrderRouter` |
| `RT_ORDER_COMMISSION` | Commission reports | `OrderRouter::commissionReceived` | **MIGRATED** — `IBComClientImpl::commissionReport()` forwards to `OrderRouter` |
| `RT_NEXT_VALID_ID` | Next valid order ID | `OrderRouter::nextValidIdReceived` | **MIGRATED** — `IBComClientImpl::nextValidId()` forwards to `OrderRouter` |
| `RT_REQ_ACCOUNT_SUMMURY` | Account summary data | `AccountRouter::accountSummaryUpdated` | **MIGRATED** — `IBComClientImpl::accountSummary()` and `accountSummaryEnd()` forward to `AccountRouter` |
| `RT_TICK_GENERIC` | Generic tick data | — | **NOT MIGRATED** — Not used by pipeline blocks |
| `RT_TICK_STRING` | Tick string data | — | **NOT MIGRATED** — Not used by pipeline blocks |
| `RT_HISTORICAL_TICK_DATA` | Historical tick data | — | **NOT MIGRATED** — Low priority, not used by pipeline blocks |
| `RT_MKT_DEPTH` | Market depth L1 | — | **NOT MIGRATED** — Low priority |
| `RT_MKT_DEPTH_L2` | Market depth L2 | — | **NOT MIGRATED** — Low priority |
| `RT_REQ_OPTION_PRICE` | Option price calc | — | **NOT MIGRATED** — Not applicable to pipeline strategy types |
| `RT_REQ_RESTART_SUBSCRIPTION` | Subscription restart | — | **NOT MIGRATED** — Infrastructure concern, not data routing |
| `RT_REQ_ERROR_SUBSRIPTION` | Error notification | — | **NOT MIGRATED** — Infrastructure concern |
| `RT_REQ_ORDER_STATUS` | Order status subscription | — | **REPLACED** by `OrderRouter` (direct forwarding) |
| `RT_REQ_CUR_TIME` | Current time | — | **NOT MIGRATED** — Low priority |
| `RT_REQ_REL_DATA` | Real-time data subscription | — | **REPLACED** by `MarketDataRouter` (direct forwarding) |

## CDispatcher Subscribers

| Subscriber Class | Messages Consumed | Migration Path |
|---|---|---|
| `CBasicRoot` | `RT_NEXT_VALID_ID` | → `OrderRouter::nextValidIdReceived` — legacy strategy root can subscribe to `OrderRouter` signal |
| `CBasicAccount` | `RT_REQ_ACCOUNT_SUMMURY` | → `AccountRouter::accountSummaryUpdated` |
| `CBasicPortfolio` | `RT_REQ_POSITION` | → `PositionRouter::positionChanged` |
| `CProcessingBase_v2` | All `RT_*` types (main message handler) | Pipeline strategies bypass this entirely via typed routers |
| `AlphaModGetTime` | Timer-based only | No dispatcher dependency |
| `CBrokerDataProvider` | Subscription management | Keep until full `CDispatcher` removal |

## Retirement Roadmap

### Phase A: Dual-Forward (CURRENT)
All typed routers run in parallel with `CDispatcher`. `IBComClientImpl` forwards each callback to both systems.

### Phase B: New Strategies Use Only Routers
All new pipeline strategies are wired exclusively through typed routers. Legacy strategies continue to use `CDispatcher`.

### Phase C: Migrate Legacy Strategies
- `cMomentum` → `MomentumAlphaBlock` in pipeline (migration config available: `legacy_momentum_pipeline.json`)
- `CMovingAverageCrossover` → `MovingAverageCrossoverAlphaBlock` in pipeline (migration config available: `ma_crossover_pipeline.json`)
- `csma` → Requires new SMA alpha block
- `cteststrategy` → Test adapter

### Phase D: Remove CDispatcher
Once all subscribers are migrated, remove:
- `CDispatcher` class and `Dispatcher.h/.cpp`
- `CSubscriber` base class
- `CProcessingBase_v2::MessageHandler()` callback
- `SendMessageToSubscribers()` calls from `IBComClientImpl`
- `CBrokerDataProvider` subscription management can be simplified

### Not-Yet-Migrated Message Types
The following message types are low priority and not needed by current pipeline blocks:
- `RT_TICK_GENERIC`, `RT_TICK_STRING` — specialized tick data, rarely used
- `RT_HISTORICAL_TICK_DATA` — historical ticks (different from historical bars)
- `RT_MKT_DEPTH`, `RT_MKT_DEPTH_L2` — market depth data
- `RT_REQ_OPTION_PRICE` — options-specific

These will be migrated when pipeline blocks requiring this data are created.
