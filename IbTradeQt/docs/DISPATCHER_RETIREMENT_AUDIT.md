# CDispatcher Retirement Audit

**Date**: 2026-03-04
**Status**: **RETIRED** -- CDispatcher has been fully removed. All data flows through typed Qt signal/slot routers.

## Message Type Migration Status


| Message Type                  | Description             | Typed Router                                                   | Status                                                                                            |
| ----------------------------- | ----------------------- | -------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| `RT_NEXT_VALID_ID`            | Next valid order ID     | `OrderRouter::nextValidIdReceived`                             | **Migrated (Phase B)** -- Legacy models receive via `CProcessingBase_v2::slotRouterNextValidId()` |
| `RT_REQ_ACCOUNT_SUMMURY`      | Account summary data    | `AccountRouter::accountSummaryUpdated`                         | **Migrated (Phase B)** -- Adapter slot converts `AccountSummaryData` -> `CAccountSummary`         |
| `RT_REQ_POSITION`             | Position updates        | `PositionRouter::positionChanged` / `positionSnapshotComplete` | **Migrated (Phase B)** -- Adapter slots populate `m_positionMap`, emit `signalEndRecvPosition`    |
| `RT_HISTORICAL_DATA`          | Historical bar data     | `HistoricalDataRouter::barsReceived`                           | **Migrated (Phase B)** -- Adapter slot converts `HistoricalBar` -> `CHistoricalData`              |
| `RT_TICK_PRICE`               | Tick price updates      | `MarketDataRouter::tick`                                       | **Migrated (Phase D)** -- CDispatcher forwarding removed                                          |
| `RT_TICK_SIZE`                | Tick size updates       | `MarketDataRouter::tickSizeUpdate`                             | **Migrated (Phase D)** -- CDispatcher forwarding removed                                          |
| `RT_REALTIME_BAR`             | 5-second real-time bars | `MarketDataRouter::barClose`                                   | **Migrated (Phase D)** -- CDispatcher forwarding removed                                          |
| `RT_TICK_BY_TICK_DATA`        | Tick-by-tick trade data | `MarketDataRouter::tickByTickTrade`                            | **Migrated (Phase D)** -- CDispatcher forwarding removed                                          |
| `RT_ORDER_STATUS`             | Order status changes    | `OrderRouter::orderStatusChanged`                              | **Migrated (Phase D)** -- CDispatcher forwarding removed                                          |
| `RT_ORDER_EXECUTION`          | Execution details       | `OrderRouter::executionReceived`                               | **Migrated (Phase D)** -- Adapter slot converts to `CExecutionReport`                             |
| `RT_ORDER_COMMISSION`         | Commission reports      | `OrderRouter::commissionReceived`                              | **Migrated (Phase D)** -- Adapter slot converts to `CCommissionReport`                            |
| `RT_REQ_CUR_TIME`             | Current time            | `TimeRouter::currentTimeReceived`                              | **Migrated (Phase D)** -- New `TimeRouter`, `AlphaModGetTime` connects directly                   |
| `RT_REQ_RESTART_SUBSCRIPTION` | Subscription restart    | `MarketDataRouter::subscriptionRestarted`                      | **Migrated (Phase D)** -- Infrastructure signal on `MarketDataRouter`                             |
| `RT_REQ_ERROR_SUBSRIPTION`    | Error notification      | `MarketDataRouter::subscriptionError`                          | **Migrated (Phase D)** -- Infrastructure signal on `MarketDataRouter`                             |
| `RT_TICK_GENERIC`             | Generic tick data       | `MarketDataRouter::tickGenericReceived`                        | **Migrated (Phase E)** -- `GenericTick` Q_GADGET struct via `onTickGeneric()`                     |
| `RT_TICK_STRING`              | Tick string data        | `MarketDataRouter::tickStringReceived`                         | **Migrated (Phase E)** -- `StringTick` Q_GADGET struct via `onTickString()`                       |
| `RT_HISTORICAL_TICK_DATA`     | Historical tick data    | `HistoricalDataRouter::historicalTicksLastReceived`             | **Migrated (Phase E)** -- `HistoricalTickLast` Q_GADGET struct via `onHistoricalTicksLast()`      |
| `RT_MKT_DEPTH`                | Market depth L1         | `MarketDepthRouter::depthUpdated`                              | **Migrated (Phase E)** -- `DepthUpdate` Q_GADGET struct via new `MarketDepthRouter`               |
| `RT_MKT_DEPTH_L2`             | Market depth L2         | `MarketDepthRouter::depthL2Updated`                            | **Migrated (Phase E)** -- `DepthL2Update` Q_GADGET struct via new `MarketDepthRouter`             |
| `RT_REQ_OPTION_PRICE`         | Option price calc       | `MarketDataRouter::optionComputationReceived`                  | **Migrated (Phase E)** -- `OptionComputation` Q_GADGET struct via `onTickOptionComputation()`     |


## Former CDispatcher Subscribers


| Former Subscriber Class | Migration Status                                                                                                |
| ----------------------- | --------------------------------------------------------------------------------------------------------------- |
| `CProcessingBase_v2`    | **Fully migrated** -- `CSubscriber` inheritance removed. Receives all data through typed router adapter slots   |
| `CBasicRoot`            | **Fully migrated** -- Receives `nextValidId` via `OrderRouter` adapter slot                                     |
| `CBasicAccount`         | **Fully migrated** -- Receives account summary and positions via `AccountRouter`/`PositionRouter` adapter slots |
| `CBasicAlphaModel`      | **Fully migrated** -- Receives historical data via `HistoricalDataRouter` adapter slot                          |
| `AlphaModGetTime`       | **Fully migrated** -- `CSubscriber` removed. Receives time via `TimeRouter::currentTimeReceived`                |
| `CPresenter`            | **Fully migrated** -- `CSubscriber` inheritance removed                                                         |
| `BaseImpl`              | **Fully migrated** -- `CSubscriber` inheritance removed                                                         |
| `CBrokerDataProvider`   | **Fully migrated** -- No longer inherits `CDispatcher`. Uses standalone `GlobalReqManager`                      |


## Deleted Files


| File                    | Reason                                           |
| ----------------------- | ------------------------------------------------ |
| `IBComm/Dispatcher.h`   | CDispatcher class definition -- no longer needed |
| `IBComm/Dispatcher.cpp` | CDispatcher implementation -- no longer needed   |


## Phase D Implementation Details

### TimeRouter

New `IBComm::TimeRouter` class routes `IBComClientImpl::currentTime()` to `AlphaModGetTime::slotCurrentTimeReceived()`. Created in `CApplicationController`, set on `IBComClientImpl` and `CBrokerDataProvider`.

### Infrastructure Signals on MarketDataRouter

`MarketDataRouter` gained `subscriptionRestarted()` and `subscriptionError(int, int, QString)` signals to handle `RT_REQ_RESTART_SUBSCRIPTION` and `RT_REQ_ERROR_SUBSRIPTION` without CDispatcher.

### Adapter Slots in CProcessingBase_v2

Phase D added two new adapter slots:

- `slotRouterExecution(ExecutionReport)` -> `CExecutionReport` -> `signalRecvExecutionReport`
- `slotRouterCommission(CommissionUpdate)` -> `CCommissionReport` -> `signalRecvCommissionReport`

These join the Phase B adapter slots:

- `slotRouterNextValidId(int)` -> `setNextValidId()`
- `slotRouterAccountSummary(AccountSummaryData)` -> `CAccountSummary` -> `signalRecvAccountSummary`
- `slotRouterPositionChanged(PositionUpdate)` -> `CPosition` -> `m_positionMap`
- `slotRouterPositionSnapshotComplete()` -> `signalEndRecvPosition`
- `slotRouterBarsReceived(int, QString, QVector<HistoricalBar>)` -> `QList<CHistoricalData>` -> `signalCbkRecvHistoricalData`

### CBrokerDataProvider Decoupled

- No longer inherits `CDispatcher`
- `GlobalReqManager` is now a standalone member
- All request/cancel methods operate without `CSubscriberPtr` parameters
- Router pointers (`MarketDataRouter`*, `OrderRouter*`, etc.) stored and propagated to models

### Phase E: Full Typed Router Coverage

All 6 previously-unused message types now have Q_GADGET structs and typed router signals:

- `MarketDataRouter` extended with `GenericTick`, `StringTick`, `OptionComputation` structs and corresponding signals
- New `MarketDepthRouter` created with `DepthUpdate`, `DepthL2Update` structs for L1/L2 order book data
- `HistoricalDataRouter` extended with `HistoricalTickLast` struct for historical tick data
- `IBComClientImpl` callbacks wired to emit through typed routers (replacing no-op stubs)
- `MarketDepthRouter` stored on `IBComClientImpl`, `CBrokerDataProvider`, created in `CApplicationController`

### Test Coverage

309 tests across 21 suites, including:

- Phase B integration tests (8 tests): Router-to-legacy-type conversion
- Phase D integration tests (13 tests): Exclusive typed router flow, TimeRouter, infrastructure signals, legacy type conversion
- Phase E integration tests (7 tests): All 6 new typed router signals, field mapping, fallback symbol resolution

