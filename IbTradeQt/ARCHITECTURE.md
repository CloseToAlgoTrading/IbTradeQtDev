# IbTradeQt Architecture Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [High-Level Architecture](#high-level-architecture)
3. [Component Architecture](#component-architecture)
4. [Design Patterns](#design-patterns)
5. [Strategy Framework Architecture](#strategy-framework-architecture)
6. [Data Flow Architecture](#data-flow-architecture)
7. [Threading Architecture](#threading-architecture)
8. [Database Architecture](#database-architecture)
9. [UI Architecture](#ui-architecture)
10. [Key Classes Reference](#key-classes-reference)

---

## Project Overview

**IbTradeQt** is a sophisticated Qt-based algorithmic trading platform designed for automated trading through Interactive Brokers (IB) TWS API. The system provides a flexible framework for implementing, testing, and executing multiple trading strategies with comprehensive market data management and portfolio tracking.

### Key Features
- Hierarchical portfolio and strategy management
- Real-time and historical market data processing
- Modular strategy pipeline architecture
- Multi-threaded execution for responsive UI
- Dual database system for market data and trading state
- Custom charting widgets for visualization
- Comprehensive order management and execution tracking

### Technology Stack

| Component | Technology |
|-----------|-----------|
| **GUI Framework** | Qt 6.x (QtCore, QtWidgets, QtCharts) |
| **Language** | C++17 |
| **Build System** | qmake (`.pro` files) |
| **Broker API** | Interactive Brokers TWS C++ API |
| **Market Data Storage** | PostgreSQL |
| **State Persistence** | SQLite |
| **Charting** | Qt Charts with custom candlestick widgets |
| **Decimal Precision** | Intel Binary Decimal Library (libbid) |

### Entry Point

The application starts in [`main.cpp`](main.cpp):
1. Initializes logging system (`MyLogger`)
2. Creates Qt application
3. Instantiates `CApplicationController`
4. Sets up MVC components via `setUpApplication()`
5. Shows main window and enters Qt event loop

---

## High-Level Architecture

IbTradeQt follows a **layered architecture** with clear separation of concerns:

```mermaid
flowchart TB
    subgraph presentation [Presentation Layer]
        MainWindow[Main Window UI]
        StrategyForms[Strategy Forms]
        TreeViews[Tree Views]
        Charts[Custom Charts]
    end
    
    subgraph business [Business Logic Layer]
        Controller[Application Controller]
        Presenter[Presenter]
        StrategyFramework[Strategy Framework]
        StateMachine[State Machine]
    end
    
    subgraph data [Data Access Layer]
        DBManager[Database Manager]
        DBHandler[SQLite Handler]
        DBConnector[PostgreSQL Connector]
    end
    
    subgraph external [External Communication Layer]
        BrokerProvider[Broker Data Provider]
        IBClient[IB Client Implementation]
        Dispatcher[Message Dispatcher]
    end
    
    subgraph infrastructure [Infrastructure]
        Logger[Logging System]
        ReqManager[Request Manager]
        Threading[Thread Management]
    end
    
    presentation --> business
    business --> data
    business --> external
    external --> data
    business --> infrastructure
    external --> infrastructure
```

### Layer Responsibilities

**Presentation Layer:**
- Qt-based GUI components
- User input handling
- Data visualization (trees, charts, logs)
- No business logic

**Business Logic Layer:**
- Strategy execution and management
- Portfolio hierarchy management
- Market signal processing
- Risk management and position sizing

**Data Access Layer:**
- Database operations (SQLite for state, PostgreSQL for market data)
- Thread-safe database access
- Query management

**External Communication Layer:**
- Interactive Brokers API integration
- Market data subscription management
- Order placement and tracking
- Observer pattern for data distribution

---

## Component Architecture

The project is organized into distinct modules with well-defined responsibilities:

```mermaid
graph TB
    subgraph MainSystem
        AppController[CApplicationController]
        Presenter[CPresenter]
        MainView[CIBTradeSystemView]
        MainModel[CMainModel]
    end
    
    subgraph IBComm
        BrokerProvider[CBrokerDataProvider]
        IBClientImpl[IBComClientImpl]
        DispatcherComp[CDispatcher]
        IBWorkerComp[IBworker]
    end
    
    subgraph Strategies
        StrategyFactory[CStrategyFactory]
        BaseModel[CBaseModel]
        BasicRoot[CBasicRoot]
        BasicStrategy[CBasicStrategy_V2]
        SubModels[Sub-Models]
    end
    
    subgraph DB
        DBMgr[DBManager]
        DBHndlr[DBHandler]
    end
    
    subgraph CObjects
        MarketData[Market Data Objects]
        OrderObjects[Order Objects]
        AccountData[Account Objects]
    end
    
    subgraph Common
        ProcessingBase[CProcessingBase_v2]
        Subscriber[CSubscriber Interface]
    end
    
    AppController --> Presenter
    AppController --> MainView
    AppController --> MainModel
    Presenter --> BrokerProvider
    Presenter --> IBWorkerComp
    
    MainModel --> BasicRoot
    BasicRoot --> Strategies
    
    BrokerProvider --> IBClientImpl
    BrokerProvider --> DispatcherComp
    
    Strategies --> ProcessingBase
    ProcessingBase --> Subscriber
    Subscriber --> DispatcherComp
    
    Strategies --> DBMgr
    DBMgr --> DBHndlr
    
    IBClientImpl --> CObjects
    DispatcherComp --> CObjects
    Strategies --> CObjects
```

### Directory Structure

```
IbTradeQt/
├── MainSystem/          # Application core (MVC/MVP)
│   ├── capplicationcontroller.h/cpp   # Application lifecycle
│   ├── cpresenter.h/cpp               # MVP Presenter
│   ├── ibtradesystemview.h/cpp/.ui    # Main window
│   ├── cmainmodel.h/cpp               # Application model
│   └── portfolioconfigmodel.h/cpp     # Tree view model
│
├── IBComm/              # Interactive Brokers communication
│   ├── BrokerDataProvider.h/cpp       # Broker API facade
│   ├── IBComClientImpl.h/cpp          # IB client implementation
│   ├── Dispatcher.h/cpp               # Observer/publisher
│   └── IBworker.h/cpp                 # Worker thread
│
├── Strategies/          # Trading strategy framework
│   ├── Generic/         # Base framework
│   │   ├── cbasemodel.h/cpp           # Base model class
│   │   ├── cbasicstrategy_V2.h/cpp    # Base strategy
│   │   ├── cstrategyfactory.h/cpp     # Factory
│   │   ├── UnifiedModelData.h         # Pipeline data structure
│   │   └── [sub-models]               # Selection, Alpha, Risk, etc.
│   ├── PairTrader/      # Pair trading strategy
│   ├── AutoDeltAlignment/ # Delta hedging strategy
│   └── StateMachine/    # Model state management
│
├── DB/                  # Database abstraction
│   ├── dbmanager.h/cpp              # Database manager
│   ├── dbhandler.h/cpp              # SQLite handler
│   └── dbquery.h                    # Query definitions
│
├── CObjects/            # Common data objects
│   ├── ctickprice.h/cpp             # Price tick data
│   ├── chistoricaldata.h/cpp        # Historical data
│   ├── cposition.h/cpp              # Position data
│   └── [other data objects]
│
├── Common/              # Shared utilities
│   ├── cprocessingbase_v2.h/cpp     # Base processing class
│   ├── GlobalDef.h                  # Global definitions
│   └── NHelper.h/cpp                # Helper functions
│
├── ReqManager/          # Request ID management
│   └── globalreqmanager.h/cpp       # Request tracking
│
├── Logger/              # Logging infrastructure
│   └── mylogger.h/cpp               # Custom logger
│
├── CustomWidgets/       # Custom Qt widgets
│   └── qcandlestickchart.h/cpp      # Candlestick chart
│
└── Brokers/IB/          # IB TWS API SDK
    ├── Shared/          # IB client library
    └── addon/           # Helper classes
```

---

## Design Patterns

The system employs multiple design patterns for flexibility and maintainability:

### 1. Model-View-Presenter (MVP)

**Implementation:**
- **Model**: [`CMainModel`](MainSystem/cmainmodel.h) - Application data
- **View**: [`CIBTradeSystemView`](MainSystem/ibtradesystemview.h) - Main window
- **Presenter**: [`CPresenter`](MainSystem/cpresenter.h) - Mediator between view and model
- **Controller**: [`CApplicationController`](MainSystem/capplicationcontroller.h) - Application orchestrator

**Benefits:**
- Testable business logic (presenter is independent of UI)
- Clear separation of concerns
- Facilitates parallel UI and logic development

### 2. Observer/Subscriber Pattern

**Implementation:**
- **Observable**: [`CDispatcher`](IBComm/Dispatcher.h) - Publishes market data
- **Observer**: [`CSubscriber`](Common/cprocessingbase_v2.h) - Receives data callbacks
- **Facade**: [`CBrokerDataProvider`](IBComm/BrokerDataProvider.h) - Wraps dispatcher

**Key Methods:**
- `Subscribe(CSubscriber*, TickerId, tEReqType)` - Register observer
- `Unsubscribe(CSubscriber*, TickerId)` - Deregister observer
- `SendMessageToSubscribers(void*, TickerId, tEReqType)` - Notify observers

**Benefits:**
- Decouples data producers from consumers
- Multiple strategies can subscribe to same data
- Thread-safe notifications via mutex

### 3. Composite Pattern

**Implementation:**
The strategy hierarchy uses composite pattern where each node can contain children:

```
CBasicRoot (ROOT)
  └─ CBasicAccount (ACCOUNT)
      └─ CBasicPortfolio (PORTFOLIO)
          └─ CBasicStrategy_V2 (STRATEGY)
              ├─ CBasicSelectionModel
              ├─ CBasicAlphaModel
              ├─ CBaseRebalanceModel
              ├─ CBasicRiskModel
              └─ CBasicExecutionModel
```

**Common Interface**: [`CGenericModelApi`](Strategies/Generic/cgenericmodelApi.h)

**Benefits:**
- Uniform treatment of individual strategies and portfolios
- Flexible hierarchy construction
- Recursive operations (start/stop all strategies)

### 4. Factory Pattern

**Implementation**: [`CStrategyFactory`](Strategies/Generic/cstrategyfactory.h)

Creates strategy instances based on `ModelType` enum:
- `ROOT`, `ACCOUNT`, `PORTFOLIO`, `STRATEGY`
- Specific strategies: `STRATEGY_MA`, `STRATEGY_MOMENTUM`, `STRATEGY_BASIC_TEST`
- Sub-models: `STRATEGY_SELECTION_MODEL`, `STRATEGY_ALPHA_MODEL`, etc.

**Benefits:**
- Centralized object creation
- Easy to add new strategy types
- Decouples client code from concrete classes

### 5. Pipeline/Chain of Responsibility

**Implementation:**
Strategy sub-models form a processing pipeline:

```mermaid
flowchart LR
    Selection[Selection Model] -->|DataListPtr| Alpha[Alpha Model]
    Alpha -->|DataListPtr| Rebalance[Rebalance Model]
    Rebalance -->|DataListPtr| Risk[Risk Model]
    Risk -->|DataListPtr| Execution[Execution Model]
```

Each model:
1. Receives `DataListPtr` with market signals
2. Processes/filters/enhances data
3. Emits `dataProcessed(DataListPtr)` signal to next stage

**Benefits:**
- Modular, testable components
- Easy to swap implementations
- Clear data transformation flow

### 6. State Machine Pattern

**Implementation**: [`CModelState`](Strategies/StateMachine/cmodelstate.h)

Each model has a lifecycle state machine:

```mermaid
stateDiagram-v2
    [*] --> Init
    Init --> Init2: Initialize Resources
    Init2 --> Ready: Configuration Complete
    Ready --> Running: Start Command
    Running --> Ready: Stop Command
    Ready --> [*]: Shutdown
```

**States**: `MS_Init` → `MS_Init2` → `MS_Ready` → `MS_Running`

**Benefits:**
- Well-defined initialization sequence
- Prevents invalid state transitions
- Easier debugging and logging

### 7. Singleton Pattern

**Implementation:**
- [`GlobalReqManager`](ReqManager/globalreqmanager.h) - Request ID management
- Logger systems

**Note**: Singletons used sparingly for truly global resources.

---

## Strategy Framework Architecture

The strategy framework is the core of IbTradeQt, providing a flexible, composable system for algorithmic trading.

### Hierarchical Structure

```mermaid
flowchart TB
    Root[CBasicRoot<br/>ROOT Node]
    Account1[CBasicAccount<br/>Account 1]
    Account2[CBasicAccount<br/>Account 2]
    Portfolio1[CBasicPortfolio<br/>Portfolio 1]
    Portfolio2[CBasicPortfolio<br/>Portfolio 2]
    Strategy1[CBasicStrategy_V2<br/>Moving Average]
    Strategy2[CBasicStrategy_V2<br/>Momentum]
    Strategy3[CBasicStrategy_V2<br/>Test Strategy]
    
    Root --> Account1
    Root --> Account2
    Account1 --> Portfolio1
    Account2 --> Portfolio2
    Portfolio1 --> Strategy1
    Portfolio1 --> Strategy2
    Portfolio2 --> Strategy3
    
    style Root fill:#e1f5ff
    style Account1 fill:#fff4e1
    style Account2 fill:#fff4e1
    style Portfolio1 fill:#e8f5e9
    style Portfolio2 fill:#e8f5e9
    style Strategy1 fill:#f3e5f5
    style Strategy2 fill:#f3e5f5
    style Strategy3 fill:#f3e5f5
```

### Base Class Hierarchy

```mermaid
classDiagram
    class CGenericModelApi {
        <<interface>>
        +setParent()*
        +addChild()*
        +getChild()*
        +getModelType()*
        +toJson()*
        +fromJson()*
    }
    
    class CProcessingBase_v2 {
        +MessageHandler()*
        +UnsubscribeHandler()*
        #m_pBrokerDataProvider
        #m_nextValidId
    }
    
    class CBaseModel {
        +addChild()
        +removeChild()
        +toJson()
        +fromJson()
        +StartProcessing()
        +StopProcessing()
        #m_ParametersMap
        #m_assetList
        #m_dbManager
    }
    
    class CBasicRoot {
        +getModelType() ROOT
    }
    
    class CBasicAccount {
        +getModelType() ACCOUNT
    }
    
    class CBasicPortfolio {
        +getModelType() PORTFOLIO
    }
    
    class CBasicStrategy_V2 {
        +getModelType() STRATEGY
        +setSubModels()
    }
    
    class ConcreteStrategies {
        cMomentum
        CMovingAverageCrossover
        CTestStrategy
    }
    
    CGenericModelApi <|.. CBaseModel
    CProcessingBase_v2 <|-- CBaseModel
    CBaseModel <|-- CBasicRoot
    CBaseModel <|-- CBasicAccount
    CBaseModel <|-- CBasicPortfolio
    CBaseModel <|-- CBasicStrategy_V2
    CBasicStrategy_V2 <|-- ConcreteStrategies
```

### Strategy Pipeline Architecture

Each strategy can be decomposed into five specialized sub-models:

```mermaid
flowchart LR
    subgraph StrategyPipeline[Strategy Processing Pipeline]
        direction LR
        Selection[Selection Model<br/>Asset Universe]
        Alpha[Alpha Model<br/>Signal Generation]
        Rebalance[Rebalance Model<br/>Portfolio Rebalancing]
        Risk[Risk Model<br/>Position Sizing]
        Execution[Execution Model<br/>Order Placement]
        
        Selection -->|UnifiedModelData| Alpha
        Alpha -->|UnifiedModelData| Rebalance
        Rebalance -->|UnifiedModelData| Risk
        Risk -->|UnifiedModelData| Execution
    end
    
    MarketData[Market Data] --> Selection
    MarketData --> Alpha
    Execution --> Orders[Order System]
```

### UnifiedModelData Structure

Data flows between pipeline stages using a standardized structure defined in [`UnifiedModelData.h`](Strategies/Generic/UnifiedModelData.h):

```cpp
struct UnifiedModelData {
    QString symbol;           // Asset symbol
    eDirection direction;     // LONG/SHORT/UNDEFINED
    double probability;       // Signal confidence
    double amount;           // Quantity
    double currentPrice;     // Current market price
}
```

### Strategy Factory

[`CStrategyFactory`](Strategies/Generic/cstrategyfactory.h) creates all model types:

**Available Model Types:**
- `ROOT` → `CBasicRoot`
- `ACCOUNT` → `CBasicAccount`
- `PORTFOLIO` → `CBasicPortfolio`
- `STRATEGY` → `CBasicStrategy_V2`
- `STRATEGY_MA` → `CMovingAverageCrossover`
- `STRATEGY_MOMENTUM` → `cMomentum`
- `STRATEGY_BASIC_TEST` → `CTestStrategy`
- `STRATEGY_SELECTION_MODEL` → `CBasicSelectionModel`
- `STRATEGY_ALPHA_MODEL` → `CBasicAlphaModel`
- `STRATEGY_REBALANCE_MODEL` → `CBaseRebalanceModel`
- `STRATEGY_RISK_MODEL` → `CBasicRiskModel`
- `STRATEGY_EXECTION_MODEL` → `CBasicExecutionModel`

### Strategy Configuration

Strategies store configuration in `QVariantMap` structures:
- **m_ParametersMap**: Strategy parameters (thresholds, periods, etc.)
- **m_assetList**: Asset universe
- **m_genericInfo**: Metadata and runtime info

**Persistence:**
- JSON serialization via `toJson()`/`fromJson()`
- Saved to file: `model_tree_config.json`
- Database storage via [`DBManager`](DB/dbmanager.h)

---

## Data Flow Architecture

### Interactive Brokers Connection Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant Presenter as CPresenter
    participant Worker as IBworker Thread
    participant Client as IBComClientImpl
    participant Socket as EClientSocket
    participant IB as IB Server
    
    App->>Presenter: User clicks Connect
    Presenter->>Worker: Start thread
    Worker->>Client: Create instance
    Client->>Socket: Create socket
    Client->>Socket: eConnect(host, port, clientId)
    Socket->>IB: TCP connection
    IB-->>Socket: Connected
    Socket-->>Client: isConnected()
    Client->>Worker: Start EReader loop
    
    loop Message Processing
        Worker->>Socket: processMsgs()
        Socket->>IB: Read messages
        IB-->>Socket: Market data, callbacks
        Socket-->>Client: Invoke EWrapper callbacks
    end
```

**Connection Details:**
- **Host**: `127.0.0.1` (localhost)
- **Port**: `4002` (IB Gateway) or `7497` (TWS Demo)
- **Client ID**: `1`
- **Configuration**: [`Common/GlobalDef.h`](Common/GlobalDef.h)

### Market Data Subscription Flow

```mermaid
sequenceDiagram
    participant Strategy as Strategy Model
    participant Provider as CBrokerDataProvider
    participant Dispatcher as CDispatcher
    participant Client as IBComClientImpl
    participant ReqMgr as GlobalReqManager
    participant IB as IB Server
    
    Strategy->>Provider: requestRealTimeBars(symbol)
    Provider->>Strategy: Subscribe(this, reqId)
    Provider->>Dispatcher: Register subscriber
    Provider->>ReqMgr: Generate unique reqId
    Provider->>Client: reqRealTimeBarsAPI(reqId, config)
    Client->>IB: Request market data
    
    loop Real-time Updates
        IB-->>Client: realtimeBar callback
        Client->>Dispatcher: SendMessageToSubscribers(data, reqId)
        Dispatcher->>Strategy: MessageHandler(data, RT_REALTIME_BAR)
        Strategy->>Strategy: Process bar data
    end
```

### Complete Data Flow: Price Update to Order

```mermaid
flowchart TB
    subgraph IBThread[IB Worker Thread]
        EReader[EReader<br/>Socket Reader]
        Callbacks[EWrapper Callbacks<br/>tickPrice, realtimeBar]
        CreateObj[Create Data Objects<br/>CMyTickPrice, etc.]
        DispatchCall[CDispatcher::Send]
    end
    
    subgraph MainThread[Main/Strategy Thread]
        MsgHandler[CSubscriber::MessageHandler]
        StrategyLogic[Strategy Processing<br/>Signal Generation]
        RiskCheck[Risk Management]
        OrderGen[Order Generation]
    end
    
    subgraph DBThread[Database Thread]
        DBSave[Save to SQLite<br/>Positions, Trades]
    end
    
    IB[IB Server] -->|TCP| EReader
    EReader --> Callbacks
    Callbacks --> CreateObj
    CreateObj --> DispatchCall
    DispatchCall -->|Thread Sync| MsgHandler
    MsgHandler --> StrategyLogic
    StrategyLogic --> RiskCheck
    RiskCheck --> OrderGen
    OrderGen -->|Place Order| IB
    OrderGen -->|Save State| DBSave
```

### Request Types

The system handles multiple request types defined in [`ReqManager/reqtype.h`](ReqManager/reqtype.h):

**Market Data:**
- `RT_TICK_PRICE` - Price updates (bid/ask/last)
- `RT_TICK_SIZE` - Volume updates
- `RT_REALTIME_BAR` - 5-second bars
- `RT_HISTORICAL_DATA` - Historical OHLCV data
- `RT_TICK_BY_TICK_DATA` - Tick-by-tick trades
- `RT_MKT_DEPTH` / `RT_MKT_DEPTH_L2` - Order book data

**Account & Orders:**
- `RT_REQ_POSITION` - Position updates
- `RT_REQ_ACCOUNT_SUMMURY` - Account information
- `RT_ORDER_STATUS` - Order state changes
- `RT_ORDER_EXECUTION` - Execution reports
- `RT_ORDER_COMMISSION` - Commission reports

**Options:**
- `RT_REQ_OPTION_PRICE` - Option pricing

### Data Object Types

Market data is encapsulated in type-safe objects in [`CObjects/`](CObjects/):

| Class | Purpose | Key Fields |
|-------|---------|------------|
| `CMyTickPrice` | Price tick | tickType, price, attribs |
| `CMyTickSize` | Size tick | tickType, size, attribs |
| `CHistoricalData` | Historical bar | open, high, low, close, volume, WAP |
| `CMyPosition` | Position | account, symbol, quantity, avgCost |
| `CExecutionReport` | Trade execution | orderId, execId, shares, price, side |
| `CCommissionReport` | Commission | commission, currency, realizedPnL |
| `CAccountSummary` | Account info | key, value, currency |

---

## Threading Architecture

IbTradeQt uses a multi-threaded architecture to ensure UI responsiveness while handling real-time market data and database operations.

```mermaid
flowchart TB
    subgraph MainThread[Main GUI Thread]
        QtEventLoop[Qt Event Loop]
        MainWindow[Main Window UI]
        Presenter[CPresenter]
        Strategies[Strategy Models]
    end
    
    subgraph IBThread[IB Worker Thread]
        IBWorker[IBworker::Worker]
        EReaderLoop[EReader Loop<br/>processMsgs]
        IBClient[IBComClientImpl<br/>EWrapper Callbacks]
    end
    
    subgraph DBThread[Database Thread]
        DBManager[DBManager]
        DBHandler[DBHandler<br/>SQLite Operations]
    end
    
    subgraph TimeThread[Alpha Time Thread]
        AlphaTime[AlphaModGetTime<br/>System Time]
    end
    
    QtEventLoop --> MainWindow
    MainWindow --> Presenter
    Presenter --> Strategies
    
    Presenter -->|Start| IBWorker
    IBWorker --> EReaderLoop
    EReaderLoop --> IBClient
    IBClient -->|Queued Signal| Strategies
    
    Strategies -->|Queued Signal| DBManager
    DBManager --> DBHandler
    
    AlphaTime -->|Queued Signal| MainWindow
    AlphaTime -->|Queued Signal| Strategies
    
    style MainThread fill:#e3f2fd
    style IBThread fill:#fff3e0
    style DBThread fill:#e8f5e9
    style TimeThread fill:#f3e5f5
```

### Thread Details

#### Main GUI Thread
- **Purpose**: User interface and Qt event loop
- **Components**:
  - [`CIBTradeSystemView`](MainSystem/ibtradesystemview.h) - Main window
  - [`CPresenter`](MainSystem/cpresenter.h) - Application presenter
  - Strategy models (when processing is lightweight)
- **Communication**: Direct method calls and Qt signals

#### IB Worker Thread
- **Purpose**: Non-blocking IB API message processing
- **Implementation**: [`IBworker`](IBComm/IBworker.h)
- **Process Loop**:
  ```cpp
  void Worker::process() {
      while (!stopWorking) {
          m_pIBComClientIpml->processMessagesAPI(); // Calls EReader::processMsgs()
      }
  }
  ```
- **Synchronization**: 
  - `EReaderOSSignal` for message arrival notification
  - Qt queued connections for callbacks to main thread

#### Database Thread
- **Purpose**: Prevent blocking on SQLite operations
- **Implementation**: [`DBManager`](DB/dbmanager.h) moves [`DBHandler`](DB/dbhandler.h) to QThread
- **Operations**:
  - Insert/update positions
  - Record trades
  - Store strategy state
  - Fetch model info
- **Communication**: Qt signal/slot with `Qt::QueuedConnection`

#### Alpha Time Thread
- **Purpose**: Periodic time-based triggers
- **Implementation**: [`AlphaModGetTime`](AlphaModelGetTime/alphamodgettime.h)
- **Functionality**: Emits time signals for strategy scheduling

### Thread Synchronization

**Mechanisms Used:**
1. **Qt Signal/Slot Queued Connections**
   - Automatic thread-safe message passing
   - Event loop integration
   - Used for cross-thread communication

2. **std::mutex**
   - Protects subscriber list in [`CDispatcher`](IBComm/Dispatcher.h)
   - Critical sections for shared data structures

3. **QMutex**
   - Qt-specific mutex for some components
   - Integrates with Qt's threading model

**Thread Safety Strategy:**
- Immutable data objects where possible
- Message passing over shared memory
- Minimal locking with clear ownership

---

## Database Architecture

IbTradeQt uses two database systems for different purposes:

```mermaid
flowchart TB
    subgraph Application[Application Layer]
        Strategies[Strategy Models]
        IBClient[IB Client]
    end
    
    subgraph PostgreSQL[PostgreSQL Database<br/>Market Data Storage]
        DBConn[DBConnector]
        RTBarTable[tb_real_time_bar]
        TickTable[tb_tickbyticklast_data]
    end
    
    subgraph SQLite[SQLite Database<br/>Trading State]
        DBMgr[DBManager]
        DBHndl[DBHandler]
        PosTable[OpenPositions]
        TradesTable[Trades]
        ModelTable[ModelInfo]
        StatsTable[StrategyData]
    end
    
    IBClient -->|Real-time bars| DBConn
    DBConn --> RTBarTable
    DBConn --> TickTable
    
    Strategies -->|Signal/Slot| DBMgr
    DBMgr --> DBHndl
    DBHndl --> PosTable
    DBHndl --> TradesTable
    DBHndl --> ModelTable
    DBHndl --> StatsTable
    
    style PostgreSQL fill:#e3f2fd
    style SQLite fill:#e8f5e9
```

### PostgreSQL (Market Data)

**Purpose**: High-throughput storage of real-time market data

**Implementation**: `DBConnector` class (referenced but not in main source tree)

**Tables:**
- **tb_real_time_bar**
  - Columns: timestamp, ticker, open, high, low, close, volume, WAP, count
  - Stores 5-second real-time bars from IB
  
- **tb_tickbyticklast_data**
  - Stores tick-by-tick trade data
  - High-frequency data capture

**Usage Pattern:**
- Direct insertion as data arrives from IB
- Used for backtesting and analysis
- Separate from trading state

### SQLite (Trading State)

**Purpose**: Persistent storage of trading state, positions, and strategy configuration

**Implementation**: 
- [`DBManager`](DB/dbmanager.h) - Thread manager and signal router
- [`DBHandler`](DB/dbhandler.h) - Actual database operations

**Threading Model:**
```cpp
// DBManager constructor
DBManager::DBManager(QObject *parent) : QObject(parent) {
    QThread* thread = new QThread;
    DBHandler* handler = new DBHandler();
    handler->moveToThread(thread);  // Move to separate thread
    thread->start();
}
```

**Tables** (defined in [`DB/dbquery.h`](DB/dbquery.h)):

1. **OpenPositions**
   ```sql
   CREATE TABLE IF NOT EXISTS OpenPositions (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       strategyId INTEGER,
       symbol TEXT,
       quantity REAL,
       averagePrice REAL,
       marketPrice REAL,
       pnl REAL,
       date TEXT
   )
   ```

2. **Trades**
   ```sql
   CREATE TABLE IF NOT EXISTS Trades (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       strategyId INTEGER,
       symbol TEXT,
       quantity REAL,
       price REAL,
       pnl REAL,
       fee REAL,
       date TEXT,
       tradeType TEXT
   )
   ```

3. **ModelInfo**
   ```sql
   CREATE TABLE IF NOT EXISTS ModelInfo (
       id INTEGER PRIMARY KEY,
       modelType TEXT,
       configData TEXT
   )
   ```

4. **StrategyData**
   - Stores strategy-specific state and statistics
   - JSON-serialized strategy data

**Access Patterns:**
- **Write**: Asynchronous via signals (`signalAddNewTrade`, `signalUpdatePosition`)
- **Read**: Synchronous with callback signals (`signalModelInfoFetched`)
- **Thread Safety**: All operations queued through signal/slot mechanism

---

## Threading Architecture (Detailed)

### Thread Communication Patterns

```mermaid
sequenceDiagram
    participant Main as Main Thread<br/>Strategy Model
    participant IB as IB Thread<br/>IBComClientImpl
    participant Disp as CDispatcher<br/>Thread-safe
    participant DB as DB Thread<br/>DBHandler
    
    Note over Main,IB: Subscription Phase
    Main->>IB: Subscribe to market data
    IB->>Disp: Register subscriber
    
    Note over Main,DB: Market Data Processing
    IB->>IB: Receive price update
    IB->>Disp: SendMessageToSubscribers
    Disp->>Main: MessageHandler (queued)
    Main->>Main: Process signal
    Main->>Main: Generate order decision
    
    Note over Main,DB: State Persistence
    Main->>DB: signalAddNewTrade (queued)
    DB->>DB: Insert into Trades table
    DB->>Main: signalTradeAdded (queued)
```

### Thread Lifecycle Management

**IB Worker Thread:**
```cpp
// Started in CPresenter constructor
workerThread = new QThread;
Worker *worker = new Worker(pIBBrokerClient.data());
worker->moveToThread(workerThread);

connect(workerThread, SIGNAL(started()), worker, SLOT(process()));
connect(worker, SIGNAL(finished()), workerThread, SLOT(quit()));

workerThread->start();
```

**Database Thread:**
```cpp
// Started in DBManager constructor
QThread* dbThread = new QThread;
DBHandler* handler = new DBHandler();
handler->moveToThread(dbThread);

// Connect signals AFTER moveToThread
connect(this, &DBManager::signalAddNewTrade, 
        handler, &DBHandler::slotAddNewTrade, 
        Qt::QueuedConnection);

dbThread->start();
```

### Critical Synchronization Points

1. **CDispatcher Subscriber List**
   - Protected by `std::mutex m_Mutex`
   - Locked during subscribe/unsubscribe/dispatch operations

2. **Request ID Generation**
   - [`GlobalReqManager`](ReqManager/globalreqmanager.h) manages ID allocation
   - Thread-safe ID generation for concurrent requests

3. **Qt Signal/Slot Across Threads**
   - `Qt::QueuedConnection` ensures thread-safe delivery
   - Copies data to prevent race conditions

---

## Data Flow Architecture (Detailed)

### Complete End-to-End Flow Example

Let's trace a complete scenario: **Strategy receives price update and places order**

```mermaid
flowchart TB
    Start([User Starts Strategy])
    
    subgraph Init[Initialization]
        Subscribe[Strategy Subscribes<br/>to Market Data]
        GenReqId[Generate Request ID]
        SendReq[Send Request to IB]
    end
    
    subgraph Reception[Data Reception IB Thread]
        IBRecv[IB Sends Price Update]
        Callback[tickPrice Callback]
        CreateTick[Create CMyTickPrice Object]
        Dispatch[Dispatcher Notifies Subscribers]
    end
    
    subgraph Processing[Processing Main Thread]
        MsgHandle[MessageHandler Called]
        ExtractData[Extract Price Data]
        Pipeline[Run Strategy Pipeline]
        
        subgraph PipelineSteps[Pipeline Steps]
            CheckSelection[Selection Model<br/>Asset in universe?]
            GenSignal[Alpha Model<br/>Generate signal]
            CalcRebal[Rebalance Model<br/>Target position]
            CheckRisk[Risk Model<br/>Position size OK?]
            PrepExec[Execution Model<br/>Create order]
        end
    end
    
    subgraph Execution[Order Execution]
        PlaceOrder[Place Order via IBrokerAPI]
        OrderConf[Receive Order Confirmation]
        SaveDB[Save Trade to Database]
    end
    
    Start --> Init
    Subscribe --> GenReqId
    GenReqId --> SendReq
    SendReq --> IBRecv
    
    IBRecv --> Callback
    Callback --> CreateTick
    CreateTick --> Dispatch
    Dispatch --> MsgHandle
    
    MsgHandle --> ExtractData
    ExtractData --> Pipeline
    Pipeline --> CheckSelection
    CheckSelection --> GenSignal
    GenSignal --> CalcRebal
    CalcRebal --> CheckRisk
    CheckRisk --> PrepExec
    
    PrepExec --> PlaceOrder
    PlaceOrder --> OrderConf
    OrderConf --> SaveDB
    
    SaveDB --> End([Complete])
    
    style Init fill:#e3f2fd
    style Reception fill:#fff3e0
    style Processing fill:#e8f5e9
    style Execution fill:#fce4ec
```

### Request Management Flow

```mermaid
flowchart LR
    subgraph Strategy[Strategy Model]
        StrategyInit[Initialize Strategy]
        StrategyReq[Request Market Data]
    end
    
    subgraph Provider[Broker Data Provider]
        Subscribe[Subscribe Method]
        AllocReq[Allocate Request ID]
        StoreMap[Store Symbol Mapping]
    end
    
    subgraph ReqMgr[Global Request Manager]
        CheckAvail[Check Available ID]
        MapSymbol[Map Symbol to ReqId]
        TrackSubscriber[Track Subscriber]
    end
    
    subgraph Broker[IB Client]
        SendAPI[Send API Request]
        StoreReqType[Store Request Type]
    end
    
    StrategyInit --> StrategyReq
    StrategyReq --> Subscribe
    Subscribe --> AllocReq
    AllocReq --> CheckAvail
    CheckAvail --> MapSymbol
    MapSymbol --> TrackSubscriber
    TrackSubscriber --> StoreMap
    StoreMap --> SendAPI
    SendAPI --> StoreReqType
```

---

## UI Architecture

### Main Window Structure

The main application window ([`ibtradesystemview.ui`](MainSystem/ibtradesystemview.ui)) uses a docked layout:

```mermaid
flowchart TB
    subgraph MainWindow[Main Window CIBTradeSystemView]
        subgraph MenuBar[Menu Bar]
            OptionsMenu[Options Menu<br/>Connect, Clear Log]
            ViewMenu[View Menu<br/>Show Log, Settings]
            ConfigMenu[Configuration Menu<br/>Load/Save]
        end
        
        subgraph CentralWidget[Central Widget]
            PortfolioTree[Portfolio TreeView<br/>test_treeView]
        end
        
        subgraph DockSettings[Settings Dock Right]
            SettingsTree[Settings TreeView<br/>settingsTreeView]
        end
        
        subgraph DockLog[Log Dock Bottom]
            LogText[Log TextEdit<br/>textEdit]
        end
        
        subgraph StatusBar[Status Bar]
            ConnStatus[Connection Status]
            DataStatus[Data Feed Status]
        end
    end
    
    MenuBar --> CentralWidget
    CentralWidget --> DockSettings
    CentralWidget --> DockLog
    MainWindow --> StatusBar
```

### MVP Interaction Pattern

```mermaid
sequenceDiagram
    participant User
    participant View as CIBTradeSystemView
    participant Presenter as CPresenter
    participant Model as CMainModel
    participant Provider as CBrokerDataProvider
    
    User->>View: Click Connect Button
    View->>Presenter: signalClickConnect()
    Presenter->>Provider: Initialize connection
    Provider->>Presenter: Connection status
    Presenter->>View: Update connection indicator
    View->>User: Display connected state
    
    User->>View: Right-click tree, Add Strategy
    View->>Model: slotOnClickAddStrategy()
    Model->>Model: Create strategy via factory
    Model->>View: signalUpdateData(index)
    View->>View: Refresh tree view
    View->>User: Show new strategy node
```

### Tree View Architecture

The portfolio hierarchy is displayed using Qt's Model/View architecture:

**Model**: [`CPortfolioConfigModel`](MainSystem/portfolioconfigmodel.h)
- Extends `QAbstractItemModel`
- Represents hierarchical strategy tree
- Handles add/remove/edit operations

**View**: `QTreeView` in main window
- Displays model in tree format
- Context menu for operations
- Drag-and-drop support

**Actions:**
- Add Account, Portfolio, Strategy
- Add sub-models (Selection, Alpha, Rebalance, Risk, Execution)
- Remove node
- Edit parameters

### Signal/Slot Architecture

Key connections established in [`CPresenter::MapSignals()`](MainSystem/cpresenter.cpp):

```mermaid
flowchart LR
    subgraph UI[UI Components]
        ConnectBtn[Connect Action]
        TreeActions[Tree Context Menu]
        LogSystem[Logger]
    end
    
    subgraph Presenter[CPresenter]
        ClickHandler[onClickMyButton]
        SlotHandlers[Various Slot Handlers]
    end
    
    subgraph View[Main View]
        UpdateSlots[UI Update Slots]
        LogSlot[slotOnLogMsgReceived]
    end
    
    subgraph Model[Portfolio Model]
        ModelSlots[Add/Remove Slots]
        UpdateSignals[Update Signals]
    end
    
    ConnectBtn -->|triggered| ClickHandler
    TreeActions -->|triggered| ModelSlots
    LogSystem -->|signalAddLogMsg| LogSlot
    ModelSlots -->|signalUpdateData| UpdateSlots
```

---

## Key Classes Reference

### Application Core

| Class | File | Responsibility |
|-------|------|----------------|
| `CApplicationController` | [`MainSystem/capplicationcontroller.h`](MainSystem/capplicationcontroller.h) | Application lifecycle management, component initialization |
| `CPresenter` | [`MainSystem/cpresenter.h`](MainSystem/cpresenter.h) | MVP presenter, mediates view and model, signal mapping |
| `CIBTradeSystemView` | [`MainSystem/ibtradesystemview.h`](MainSystem/ibtradesystemview.h) | Main window view, UI component management |
| `CMainModel` | [`MainSystem/cmainmodel.h`](MainSystem/cmainmodel.h) | Application data model |
| `CPortfolioConfigModel` | [`MainSystem/portfolioconfigmodel.h`](MainSystem/portfolioconfigmodel.h) | Tree view model for portfolio hierarchy |

### IB Communication

| Class | File | Responsibility |
|-------|------|----------------|
| `IBComClientImpl` | [`IBComm/IBComClientImpl.h`](IBComm/IBComClientImpl.h) | IB TWS API client, implements EWrapper callbacks |
| `CBrokerDataProvider` | [`IBComm/BrokerDataProvider.h`](IBComm/BrokerDataProvider.h) | Broker API facade, subscription management |
| `CDispatcher` | [`IBComm/Dispatcher.h`](IBComm/Dispatcher.h) | Observer pattern implementation, message routing |
| `IBworker` | [`IBComm/IBworker.h`](IBComm/IBworker.h) | Worker thread for IB message processing |
| `IBrokerAPI` | [`IBComm/IbrokerAPI.h`](IBComm/IbrokerAPI.h) | Abstract broker interface |

### Strategy Framework

| Class | File | Responsibility |
|-------|------|----------------|
| `CGenericModelApi` | [`Strategies/Generic/cgenericmodelApi.h`](Strategies/Generic/cgenericmodelApi.h) | Pure virtual interface for all models |
| `CBaseModel` | [`Strategies/Generic/cbasemodel.h`](Strategies/Generic/cbasemodel.h) | Base implementation with common functionality |
| `CBasicRoot` | [`Strategies/Generic/cbasicroot.h`](Strategies/Generic/cbasicroot.h) | Root node of strategy hierarchy |
| `CBasicAccount` | [`Strategies/Generic/cbasicaccount.h`](Strategies/Generic/cbasicaccount.h) | Account-level container |
| `CBasicPortfolio` | [`Strategies/Generic/cbasicportfolio.h`](Strategies/Generic/cbasicportfolio.h) | Portfolio management |
| `CBasicStrategy_V2` | [`Strategies/Generic/cbasicstrategy_V2.h`](Strategies/Generic/cbasicstrategy_V2.h) | Base strategy class |
| `cMomentum` | [`Strategies/Generic/cmomentum.h`](Strategies/Generic/cmomentum.h) | Momentum strategy implementation |
| `CMovingAverageCrossover` | [`Strategies/Generic/cmovingaveragecrossover.h`](Strategies/Generic/cmovingaveragecrossover.h) | Moving average strategy |
| `CTestStrategy` | [`Strategies/Generic/cteststrategy.h`](Strategies/Generic/cteststrategy.h) | Test/debugging strategy |
| `CStrategyFactory` | [`Strategies/Generic/cstrategyfactory.h`](Strategies/Generic/cstrategyfactory.h) | Factory for creating models |

### Strategy Sub-Models

| Class | File | Responsibility |
|-------|------|----------------|
| `CBasicSelectionModel` | [`Strategies/Generic/cbasicselectionmodel.h`](Strategies/Generic/cbasicselectionmodel.h) | Asset universe selection |
| `CBasicAlphaModel` | [`Strategies/Generic/cbasicalphamodel.h`](Strategies/Generic/cbasicalphamodel.h) | Signal generation |
| `CBaseRebalanceModel` | [`Strategies/Generic/cbaserebalancemodel.h`](Strategies/Generic/cbaserebalancemodel.h) | Portfolio rebalancing |
| `CBasicRiskModel` | [`Strategies/Generic/cbasicriskmodel.h`](Strategies/Generic/cbasicriskmodel.h) | Risk management |
| `CBasicExecutionModel` | [`Strategies/Generic/cbasicexecutionmodel.h`](Strategies/Generic/cbasicexecutionmodel.h) | Order execution |

### Database Layer

| Class | File | Responsibility |
|-------|------|----------------|
| `DBManager` | [`DB/dbmanager.h`](DB/dbmanager.h) | Database thread manager, signal router |
| `DBHandler` | [`DB/dbhandler.h`](DB/dbhandler.h) | SQLite operations implementation |
| `DbTrade` | [`DB/dbquery.h`](DB/dbquery.h) | Trade data structure |
| `OpenPosition` | [`DB/dbquery.h`](DB/dbquery.h) | Position data structure |
| `DbModelInfo` | [`DB/dbquery.h`](DB/dbquery.h) | Model configuration data |

### Common Components

| Class | File | Responsibility |
|-------|------|----------------|
| `CProcessingBase_v2` | [`Common/cprocessingbase_v2.h`](Common/cprocessingbase_v2.h) | Base class for data processors, subscriber interface |
| `CSubscriber` | [`Common/cprocessingbase_v2.h`](Common/cprocessingbase_v2.h) | Observer interface for market data |
| `GlobalReqManager` | [`ReqManager/globalreqmanager.h`](ReqManager/globalreqmanager.h) | Request ID allocation and tracking |
| `MyLogger` | [`Logger/mylogger.h`](Logger/mylogger.h) | Application-wide logging |

### Data Objects (CObjects)

| Class | File | Purpose |
|-------|------|---------|
| `CMyTickPrice` | [`CObjects/ctickprice.h`](CObjects/ctickprice.h) | Price tick data |
| `CMyTickSize` | [`CObjects/cticksize.h`](CObjects/cticksize.h) | Volume tick data |
| `CHistoricalData` | [`CObjects/chistoricaldata.h`](CObjects/chistoricaldata.h) | Historical bar data |
| `CMyPosition` | [`CObjects/cposition.h`](CObjects/cposition.h) | Position information |
| `CExecutionReport` | [`CObjects/cexecutionreport.h`](CObjects/cexecutionreport.h) | Trade execution details |
| `CCommissionReport` | [`CObjects/ccommissionreport.h`](CObjects/ccommissionreport.h) | Commission and P&L |
| `CAccountSummary` | [`CObjects/caccountsummary.h`](CObjects/caccountsummary.h) | Account summary data |
| `CDeltaObject` | [`CObjects/cdeltaobject.h`](CObjects/cdeltaobject.h) | Options delta data |

---

## Strategy State Machine

Each model implements a state machine for lifecycle management:

```mermaid
stateDiagram-v2
    [*] --> Init: Model Created
    
    Init: Initialize State
    Init: - Load configuration
    Init: - Allocate resources
    
    Init --> Init2: Resources Ready
    
    Init2: Secondary Initialization
    Init2: - Subscribe to market data
    Init2: - Connect to database
    
    Init2 --> Ready: Subscriptions Active
    
    Ready: Ready State
    Ready: - Awaiting start command
    Ready: - Configuration editable
    
    Ready --> Running: User Starts
    
    Running: Running State
    Running: - Processing market data
    Running: - Generating signals
    Running: - Placing orders
    
    Running --> Ready: User Stops
    
    Ready --> [*]: Shutdown
```

**State Definitions** ([`Strategies/StateMachine/cmodelstate.h`](Strategies/StateMachine/cmodelstate.h)):
- **MS_Init**: Initial state, loading configuration
- **MS_Init2**: Secondary initialization, establishing connections
- **MS_Ready**: Initialized and ready to run, but not processing
- **MS_Running**: Actively processing market data and trading

**State Interface:**
```cpp
class CModelState {
public:
    virtual void enterState(CBaseModel* model) = 0;
    virtual void handleEvent(CBaseModel* model, const e_modelStateEvent& event) = 0;
    virtual void exitState(CBaseModel* model) = 0;
    virtual e_modelState getStateID() const = 0;
};
```

---

## Configuration and Persistence

### Configuration Storage

**Runtime Configuration:**
- Stored in `QVariantMap` structures within each model
- Three main maps:
  - `m_ParametersMap` - Strategy parameters
  - `m_assetList` - Asset universe
  - `m_genericInfo` - Metadata

**File Persistence:**
- **Format**: JSON
- **File**: `model_tree_config.json` (in application directory)
- **Methods**: `toJson()` serialization, `fromJson()` deserialization
- **Scope**: Entire strategy tree hierarchy

**Database Persistence:**
- Model configuration stored in `ModelInfo` table
- Strategy state stored in `StrategyData` table
- Automatic persistence on changes

### JSON Serialization Example

Each model implements serialization:
```cpp
// Serialize model to JSON
QJsonObject CBaseModel::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::number(m_id);
    obj["modelType"] = static_cast<int>(getModelType());
    obj["parameters"] = QJsonObject::fromVariantMap(m_ParametersMap);
    obj["assets"] = QJsonObject::fromVariantMap(m_assetList);
    
    // Serialize children
    QJsonArray children;
    for (const auto& child : m_Children) {
        children.append(child->toJson());
    }
    obj["children"] = children;
    
    return obj;
}
```

---

## Interactive Brokers API Integration

### EWrapper Callback Coverage

[`IBComClientImpl`](IBComm/IBComClientImpl.h) implements 117+ callback methods from the IB API:

**Key Callbacks by Category:**

**Market Data:**
- `tickPrice()` - Price updates
- `tickSize()` - Volume updates
- `tickString()` - String data
- `tickGeneric()` - Generic ticks
- `realtimeBar()` - 5-second bars
- `historicalData()` - Historical bars
- `updateMktDepth()` - Level 2 data

**Account & Portfolio:**
- `position()` - Position updates
- `positionEnd()` - Position list complete
- `accountSummary()` - Account information
- `updateAccountValue()` - Account value changes

**Orders:**
- `orderStatus()` - Order state changes
- `openOrder()` - Open order details
- `execDetails()` - Execution reports
- `commissionReport()` - Commission details

**Connection:**
- `connectionClosed()` - Connection lost
- `error()` - Error messages

### Request Methods

**Market Data Requests:**
- `reqRealTimeDataAPI()` - Subscribe to real-time data
- `reqRealTimeBarsAPI()` - Subscribe to 5-second bars
- `reqHistoricalDataAPI()` - Request historical data
- `reqTickByTickDataAPI()` - Tick-by-tick subscription

**Account & Position Requests:**
- `reqPositionsAPI()` - Request positions
- `reqAccountSummaryAPI()` - Request account summary
- `reqOpenOrdersAPI()` - Request open orders

**Order Operations:**
- `reqPlaceOrderAPI()` - Place new order
- `cancelOrderAPI()` - Cancel existing order
- `reqGlobalCancelAPI()` - Cancel all orders

---

## Build System

### qmake Project File

Build configuration is defined in [`ibtrading.pro`](ibtrading.pro):

**Key Settings:**
```qmake
QT += core gui charts sql printsupport network widgets
CONFIG += c++17
TEMPLATE = app
TARGET = ibtrading
```

**Platform-Specific Configuration:**
```qmake
unix: LIBS += -L$$PWD/Libs/ -lbid
win32: LIBS += -L$$PWD/Libs/win/ -lbid
```

**Include Paths:**
- IB SDK: `Brokers/IB/Shared/`
- Intel Decimal Library: `Libs/`
- Generated UI headers: `GeneratedIncludes/`

**Generated Files:**
- UI files compiled to `ui_*.h` headers
- Placed in `GeneratedIncludes/` directory
- Automatically generated by `uic` compiler

---

## Logging System

### MyLogger Architecture

[`MyLogger`](Logger/mylogger.h) provides categorized logging:

**Categories:**
- `logIBCOM` - IB communication
- `logPNL` - Profit & loss tracking
- `logMAIN` - Main application
- `logDB` - Database operations
- `logSTRATEGY` - Strategy execution

**Integration:**
- Qt logging categories (`QLoggingCategory`)
- Signal emitted for each log message: `signalAddLogMsg(QString)`
- Connected to main window text edit for display

**Usage:**
```cpp
qInfo(logSTRATEGY) << "Strategy started:" << strategyName;
qWarning(logIBCOM) << "Connection lost, attempting reconnect";
qCritical(logDB) << "Database error:" << errorMessage;
```

---

## Summary

IbTradeQt demonstrates a well-architected trading system with:

1. **Clean Separation of Concerns**
   - Layered architecture with clear boundaries
   - MVP pattern for testable business logic
   - Abstracted broker communication

2. **Flexible Strategy Framework**
   - Composite hierarchy for portfolio management
   - Modular pipeline architecture for strategy components
   - Factory pattern for easy extensibility

3. **Robust Data Handling**
   - Observer pattern for efficient data distribution
   - Dual database system optimized for different use cases
   - Thread-safe communication patterns

4. **Responsive User Interface**
   - Multi-threaded architecture prevents UI blocking
   - Real-time updates via Qt signal/slot
   - Dockable windows for customizable layout

5. **Production-Ready Features**
   - Comprehensive error handling callbacks from IB
   - State machine for controlled lifecycle
   - Persistent configuration and trade history

The architecture prioritizes flexibility, allowing multiple strategies to run concurrently within a hierarchical portfolio structure, while maintaining responsive UI and reliable data management.

---

## Appendix: File Organization

### Complete Module Breakdown

**MainSystem/** - Application Core (8 files)
- Application controller, presenter, main view
- Portfolio and settings models
- Tree view management
- Icon handling

**IBComm/** - IB Communication (6 files)
- Broker API implementation
- Message dispatcher
- Worker thread management
- Data provider facade

**Strategies/** - Strategy Framework (30+ files)
- Generic base classes and interfaces
- Concrete strategy implementations
- Sub-model implementations (Selection, Alpha, Risk, etc.)
- State machine implementation

**DB/** - Database Layer (6 files)
- Database manager and handler
- Query definitions and data structures
- Thread management

**CObjects/** - Data Objects (8+ files)
- Market data structures
- Order and execution objects
- Account summary objects

**Common/** - Utilities (4 files)
- Processing base class
- Global definitions
- Helper functions

**ReqManager/** - Request Management (3 files)
- Request ID allocation
- Request type definitions
- Global request manager

**Logger/** - Logging (2 files)
- Custom logger implementation
- Category definitions

**CustomWidgets/** - Custom UI (2+ files)
- Candlestick chart widget
- Line chart widget

**Brokers/IB/** - IB SDK (50+ files)
- TWS API C++ client library
- Helper classes and samples

**GeneratedIncludes/** - Generated UI Headers
- Auto-generated from `.ui` files
- Not manually edited

---

*This architecture documentation provides a comprehensive overview of the IbTradeQt trading system. For issues and improvement suggestions, see [ISSUES_AND_IMPROVEMENTS.md](ISSUES_AND_IMPROVEMENTS.md).*
