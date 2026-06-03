#ifndef DBDATATYPES_H
#define DBDATATYPES_H

#include <QString>
#include <QDateTime>
#include <QList>

enum e_positionStatus {
    PS_INIT_OPEN = 0,
    PS_OPEN,
    PS_CLOSED,
    PS_PARTIALY_CLOSED
};

enum e_queryStatus{
    QS_VALID = 0,
    QS_NOT_FOUND,
    QS_ERROR
};

struct OpenPosition {
    int id;            // Unique identifier, auto-incremented
    QString strategyId;// Identifier for the strategy
    QString symbol;    // Trading symbol, up to 10 characters
    int quantity;      // Quantity of the position
    double price;      // Price of the position
    double pnl;        // Profit and Loss
    double fee;        // Associated fee
    QString date;      // Date in text format
    int status;        // Status of the position (open, closed, etc.)

    // Default constructor
    OpenPosition()
        : id(0), strategyId(""), quantity(0), price(0.0), pnl(0.0), fee(0.0), date(""), status(static_cast<int>(PS_INIT_OPEN)) {}
};


struct DbTrade {
    QString execId;   // Unique identifier for thetrade execution
    QString strategyId;    // Associated Strategy ID
    QString symbol;    // Trading symbol
    int quantity;      // Quantity of the trade
    double price;      // Price at which the trade was executed
    double pnl;        // Profit and Loss for the trade
    double fee;        // Associated fee with the trade
    QString date;      // Date of the trade
    QString tradeType; // Type of the trade (e.g., BUY, SELL)

    // Default constructor
    DbTrade() : execId(""), strategyId(""), quantity(0), price(0.0), pnl(0.0), fee(0.0), tradeType("BUY") {}
};

struct DbTradeCommission {
    QString execId;   // Unique identifier for thetrade execution
    double pnl;        // Profit and Loss for the trade
    double fee;        // Associated fee with the trade

    // Default constructor
    DbTradeCommission() : execId(""), pnl(0.0), fee(0.0) {}
};


struct DbStrategyData {
    QString strategyId;    // Strategy ID
    double availableBP;    // Available Buying Power (BP)
    double usedBP;         // Used Buying Power (BP)
    double realizedPnL;    // Realized Profit & Loss (P&L)
    double unrealizedPnL;  // Unrealized Profit & Loss (P&L)
    double pnlPercentage;  // P&L Percentage
    double fees;           // Fees

    // Default constructor
    DbStrategyData()
        : strategyId(""),
        availableBP(0.0),
        usedBP(0.0),
        realizedPnL(0.0),
        unrealizedPnL(0.0),
        pnlPercentage(0.0),
        fees(0.0)
    {}
};

struct DbModelInfo {
    QString modelId;          // Strategy ID
    QString modelName;        // Strategy Name
    QString modelDescription; // Strategy Description
    QDateTime createdAt;         // Creation Timestamp
    QDateTime updatedAt;         // Last Updated Timestamp
    QString status;              // Status of the Strategy

    // Default constructor
    DbModelInfo()
        : modelId(""),
        modelName(""),
        modelDescription(""),
        createdAt(),
        updatedAt(),
        status("")
    {}
};


// ---------------------------------------------------------------------------
// Backtest DB data types
// ---------------------------------------------------------------------------

struct DbBacktestRun {
    QString runId;
    QString strategyId;          // live node UUID (legacy, kept for backward compat)
    QString strategyDisplayName;
    QString portfolioPath;
    QString configJson;
    QString symbols;
    QString startDate;
    QString endDate;
    QString status;              // "Created" | "Running" | "Finished" | "Failed"
    QString errorText;
    qint64  durationMs      = 0;
    QString engineVersion;
    QString dataSourceId;
    QString dataRefreshedAt;
    QString createdAt;
    // --- scope/definition fields ---
    QString strategyDefId;       // canonical definition UUID (legacy name, = strategy_id)
    QString scopeType   = QStringLiteral("strategy");  // "strategy" | "portfolio" | "account"
    QString scopeRefId;          // UUID of scope object
    int     strategyVersion = 1; // definition version snapshot at time of run
    // --- v3 catalog fields (dual-write alongside legacy fields) ---
    QString catalogStrategyId;   // FK → strategies.strategy_id
    QString catalogVersionId;    // FK → strategy_versions.version_id
};

struct DbBacktestMetrics {
    QString runId;
    double  totalReturn      = 0.0;
    double  annualizedReturn = 0.0;
    double  sharpeRatio      = 0.0;
    double  maxDrawdown      = 0.0;
    double  winRate          = 0.0;
    int     totalTrades      = 0;
    double  initialCapital   = 0.0;
    double  finalCapital     = 0.0;
    double  benchmarkReturn  = 0.0;
    double  benchmarkSharpe  = 0.0;
    /// Ticker used for benchmark comparison (mirrors run config / result).
    QString benchmarkSymbol;
    double  benchmarkAnnualizedReturn = 0.0;
    double  benchmarkMaxDrawdown      = 0.0;
    double  benchmarkStartPrice       = 0.0;
    double  benchmarkEndPrice         = 0.0;
    double  alpha            = 0.0;
    double  sortinoRatio           = 0.0;
    double  calmarRatio            = 0.0;
    double  profitFactor           = 0.0;
    double  averageExposurePct     = 0.0;
    double  turnoverAnnualized     = 0.0;
    int     metricDefinitionsVersion = 0;
    QString statisticsJson; ///< Full `BacktestStatistics::toJson` for monthly/yearly arrays and versioning
};

struct DbBacktestTrade {
    QString runId;
    QString symbol;
    QString side;           // "BUY" | "SELL"
    double  quantity    = 0.0;
    double  fillPrice   = 0.0;
    QString timestamp;      // UTC ISO 8601
};

struct DbBacktestEquityPoint {
    QString runId;
    QString timestamp;      // UTC ISO 8601
    double  value           = 0.0;
    double  benchmarkValue  = 0.0;
};

struct DbHistoricalBar {
    QString symbol;
    QString resolution;     // "Day1" | "Min1" | etc.
    QString dataSourceId;   // "yahoo" | "csv" | "jsonl"
    QString timestamp;      // UTC ISO 8601
    double  open    = 0.0;
    double  high    = 0.0;
    double  low     = 0.0;
    double  close   = 0.0;
    double  volume  = 0.0;
};

/// Provider-scoped instrument metadata (mutable snapshot; latest upsert wins).
struct DbInstrumentMetadata {
    QString providerSymbol;
    QString providerId;       // "yahoo" | "ib" | "csv" | …
    QString assetKind;        // Backtest::AssetKind string
    QString sourceRawType;
    QString currency;
    QString exchange;
    QString displayName;
    QString tradingScheduleId; ///< optional v1 placeholder for future session/schedule id
    QString rawJson;          // optional debug/trace; bounded in writers
    QString updatedAt;        // UTC ISO 8601
};

// Lightweight summary row returned by slotFetchRunsForStrategy
struct DbBacktestRunSummary {
    QString runId;
    QString strategyId;
    QString symbols;
    QString startDate;
    QString endDate;
    QString status;
    QString dataSourceId;
    QString createdAt;
    // Canonical scope fields (populated by query_fetchRunsForDefinition)
    QString strategyDefId;
    QString scopeType;
    QString scopeRefId;
    int     strategyVersion = 1;
    // v3 catalog fields
    QString catalogStrategyId;
    QString catalogVersionId;
    // Metrics flattened for display in Run History panel
    double  totalReturn  = 0.0;
    double  sharpeRatio  = 0.0;
};

// ---------------------------------------------------------------------------
// Strategy catalog data types (stored in ModelTreeRepository DB)
// ---------------------------------------------------------------------------

// Legacy type kept for migration and backward-compat reads of strategy_definitions_backup.
struct DbStrategyDefinition {
    QString strategyDefId;
    QString name;
    int     strategyKind    = 0;
    QString configJson;
    int     version         = 1;
    QString lifecycleState  = QStringLiteral("draft");
    bool    isArchived      = false;
    QString createdAt;
    QString updatedAt;
    QString createdFromDefId;

    bool isValid() const { return !strategyDefId.isEmpty(); }
};

// Strategy family / container — the long-lived catalog object.
struct DbStrategy {
    QString strategyId;
    QString name;
    int     strategyKind    = 0;
    QString lifecycleState  = QStringLiteral("draft");
    QString description     = QStringLiteral("");
    QString tags            = QStringLiteral("");
    bool    isArchived      = false;
    QString createdAt;
    QString updatedAt;

    bool isValid() const { return !strategyId.isEmpty(); }
};

// Immutable versioned config snapshot belonging to a strategy family.
struct DbStrategyVersion {
    QString versionId;
    QString strategyId;
    int     versionNumber   = 1;
    QString configJson      = QStringLiteral("{}");
    QString notes           = QStringLiteral("");
    QString lifecycleState  = QStringLiteral("draft");
    bool    isPublished     = false;
    QString createdFromVersionId;
    QString createdAt;

    bool isValid() const { return !versionId.isEmpty(); }
};

struct DbLiveStrategyBinding {
    QString bindingId;
    QString modelNodeId;   // FK → model_nodes.uuid
    QString strategyDefId; // FK → strategies.strategy_id (kept name for compat)
    QString versionId      = QStringLiteral("");  // FK → strategy_versions.version_id
    QString createdAt;
    QString updatedAt;

    bool isValid() const { return !bindingId.isEmpty(); }
};

// ---------------------------------------------------------------------------
// Backtest run profile (stored in ModelTreeRepository DB — accessed synchronously
// via ISystemBackend::createBacktestRunProfile / listBacktestRunProfiles)
// ---------------------------------------------------------------------------

struct DbBacktestRunProfile {
    QString profileId;
    QString ownerType;    // "strategy_definition" | "live_strategy" | "portfolio" | "account"
    QString ownerRefId;   // UUID of the owning object described by ownerType
    QString name;
    QString runConfigJson;
    QString createdAt;
    QString updatedAt;
};

#endif // DBDATATYPES_H
