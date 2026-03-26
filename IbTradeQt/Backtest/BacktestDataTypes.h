#ifndef BACKTEST_BACKTESTDATATYPES_H
#define BACKTEST_BACKTESTDATATYPES_H
#include "BacktestConstants.h"

// BacktestDataTypes.h
//
// Defines the three-tier backtest data model that keeps strategy config,
// run input, and run results cleanly separated:
//
//   BacktestProfile    — lightweight defaults stored per-strategy (3 fields only).
//                        Lives in m_pipelineConfig["backtestProfile"].
//                        Does NOT contain run-specific parameters like dates or capital.
//
//   BacktestRunConfig  — concrete one-shot run input built in the Backtest Workspace UI.
//                        Never stored in the portfolio tree. Serialised to JSON and
//                        persisted in BacktestRuns.configJson for reproducibility.
//
//   BacktestRunRecord  — persisted metadata + lifecycle status for a single run.
//                        Contains NO aggregate metrics — metrics live in BacktestMetrics.
//
//   BacktestLoadedRun  — composed in-memory view assembled after a run completes or
//                        when a past run is loaded from DB. Combines record + full
//                        BacktestResult (which carries tradeLog, equityCurve, metrics).
//
// Bar normalisation policy (enforced by HistoricalDataManager):
//   - Yahoo Finance bars are split-adjusted and dividend-adjusted as returned by the API.
//     Unadjusted close is not cached separately.
//   - All timestamps are stored as UTC ISO 8601 strings.
//   - Daily bars are anchored to market session close: 21:00 UTC (= 16:00 ET).
//   - Calendar gaps (holidays, weekends) are left as gaps in the cache.
//     Equity-curve alignment for benchmark comparison is handled at the
//     BacktestMetricsCollector level, not in the cache layer.

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QMetaType>
#include "Backtest/BacktestResult.h"
#include "DB/dbdatatypes.h"

namespace Backtest {

// ---------------------------------------------------------------------------
// BacktestProfile — stored per-strategy, 3 fields only
// ---------------------------------------------------------------------------
struct BacktestProfile {
    QString defaultBenchmark;   // e.g. "SPY"
    QString defaultResolution;  // e.g. "Day1"
    QString defaultDataSource;  // e.g. "yahoo"

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["defaultBenchmark"]  = defaultBenchmark;
        obj["defaultResolution"] = defaultResolution;
        obj["defaultDataSource"] = defaultDataSource;
        return obj;
    }

    static BacktestProfile fromJson(const QJsonObject& obj) {
        BacktestProfile p;
        p.defaultBenchmark  = obj.value("defaultBenchmark").toString("SPY");
        p.defaultResolution = obj.value("defaultResolution").toString("Day1");
        p.defaultDataSource = obj.value("defaultDataSource").toString("yahoo");
        return p;
    }
};

// ---------------------------------------------------------------------------
// BacktestRunConfig — one-shot run input, built entirely in the Backtest UI
// ---------------------------------------------------------------------------
struct BacktestRunConfig {
    QString     strategyId;          // live node UUID (legacy; kept for backward compat)
    QString     strategyDisplayName;
    QString     portfolioPath;       // e.g. "Account1/Portfolio2/MACrossover"
    QString     pipelineConfigJson;  // full pipeline JSON snapshot at time of run

    // --- canonical scope fields (see scopeRefId truth table in design) ---
    QString     strategyDefId;       // canonical definition UUID; empty if not yet bound
    QString     scopeType = QStringLiteral("strategy"); // "strategy" | "portfolio" | "account"
    QString     scopeRefId;          // UUID of scope object (def UUID or node UUID per scopeType)
    int         strategyVersion = 1; // definition version snapshot; callers should set this

    // v3 catalog fields (dual-write alongside legacy fields)
    QString     catalogStrategyId;   // FK → strategies.strategy_id
    QString     catalogVersionId;    // FK → strategy_versions.version_id

    QStringList symbols;
    QDateTime   startDate;
    QDateTime   endDate;
    double      initialCapital  = 100'000.0;
    QString     benchmarkSymbol;     // e.g. "SPY"; empty = no benchmark

    // Maps to BarResolution enum — stored as string for DB serialisation
    // Supported values: "Tick", "Sec5", "Min1", "Min5", "Min15", "Min30", "Hour1", "Day1"
    QString     resolution      = QStringLiteral("Day1");

    // Maps to FillModelType enum
    // Supported values: "Instant", "MidPrice", "BidAsk", "SlippageBps"
    QString     fillModel       = QStringLiteral("BidAsk");

    // Maps to FillTiming enum
    // Supported values: "SignalOnClose_FillNextBarOpen", "SignalOnTick_FillAtBidAsk",
    //                   "SignalOnClose_FillAtClose"
    QString     fillTiming      = QStringLiteral("SignalOnClose_FillNextBarOpen");

    double      slippageBps     = 1.0;

    // "yahoo" | "csv" | "jsonl"
    QString     dataSourceId    = QStringLiteral("yahoo");

    // Optional: compact JSON object { "SYM": { "classificationOverride": "...", ... } } for resolver.
    QString     assetListJson;

    // Optional: path/URL for csv or jsonl sources; empty for yahoo
    QString     dataPath;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["strategyId"]          = strategyId;
        obj["strategyDisplayName"] = strategyDisplayName;
        obj["portfolioPath"]       = portfolioPath;
        obj["pipelineConfigJson"]  = pipelineConfigJson;
        obj["strategyDefId"]       = strategyDefId;
        obj["scopeType"]           = scopeType;
        obj["scopeRefId"]          = scopeRefId;
        obj["strategyVersion"]     = strategyVersion;
        obj["catalogStrategyId"]   = catalogStrategyId;
        obj["catalogVersionId"]    = catalogVersionId;
        QJsonArray syms;
        for (const auto& s : symbols) syms.append(s);
        obj["symbols"]             = syms;
        obj["startDate"]           = startDate.toUTC().toString(Qt::ISODate);
        obj["endDate"]             = endDate.toUTC().toString(Qt::ISODate);
        obj["initialCapital"]      = initialCapital;
        obj["benchmarkSymbol"]     = benchmarkSymbol;
        obj["resolution"]          = resolution;
        obj["fillModel"]           = fillModel;
        obj["fillTiming"]          = fillTiming;
        obj["slippageBps"]         = slippageBps;
        obj["dataSourceId"]        = dataSourceId;
        obj["assetListJson"]       = assetListJson;
        obj["dataPath"]            = dataPath;
        return obj;
    }

    static BacktestRunConfig fromJson(const QJsonObject& obj) {
        BacktestRunConfig c;
        c.strategyId          = obj.value("strategyId").toString();
        c.strategyDisplayName = obj.value("strategyDisplayName").toString();
        c.portfolioPath       = obj.value("portfolioPath").toString();
        c.pipelineConfigJson  = obj.value("pipelineConfigJson").toString();
        c.strategyDefId       = obj.value("strategyDefId").toString();
        c.scopeType           = obj.value("scopeType").toString(QString(Backtest::Scope::Strategy));
        c.scopeRefId          = obj.value("scopeRefId").toString();
        c.strategyVersion     = obj.value("strategyVersion").toInt(1);
        c.catalogStrategyId   = obj.value("catalogStrategyId").toString();
        c.catalogVersionId    = obj.value("catalogVersionId").toString();
        QJsonArray syms       = obj.value("symbols").toArray();
        for (const auto& v : syms) c.symbols.append(v.toString());
        c.startDate           = QDateTime::fromString(obj.value("startDate").toString(), Qt::ISODate);
        c.endDate             = QDateTime::fromString(obj.value("endDate").toString(), Qt::ISODate);
        c.initialCapital      = obj.value("initialCapital").toDouble(100'000.0);
        c.benchmarkSymbol     = obj.value("benchmarkSymbol").toString();
        c.resolution          = obj.value("resolution").toString("Day1");
        c.fillModel           = obj.value("fillModel").toString("BidAsk");
        c.fillTiming          = obj.value("fillTiming").toString("SignalOnClose_FillNextBarOpen");
        c.slippageBps         = obj.value("slippageBps").toDouble(1.0);
        c.dataSourceId        = obj.value("dataSourceId").toString("yahoo");
        c.assetListJson       = obj.value("assetListJson").toString();
        c.dataPath            = obj.value("dataPath").toString();
        return c;
    }

    QString symbolsJoined() const { return symbols.join(QStringLiteral(",")); }
};

// ---------------------------------------------------------------------------
// BacktestRunRecord — persisted metadata + status, NO metrics fields
// ---------------------------------------------------------------------------
struct BacktestRunRecord {
    QString runId;              // UUID generated at run creation
    QString strategyId;         // live node UUID (legacy; kept for backward compat)
    QString strategyDisplayName;
    QString portfolioPath;
    QString configJson;         // full BacktestRunConfig serialised to JSON
    QString symbols;            // comma-separated copy for quick display queries
    QString startDate;          // UTC ISO 8601
    QString endDate;            // UTC ISO 8601

    // Lifecycle: Created → Running → Finished
    //                              ↘ Failed
    // "Cancelled" is deferred to v2.
    QString status;             // "Created" | "Running" | "Finished" | "Failed"
    QString errorText;

    qint64  durationMs      = 0;
    QString engineVersion;      // app version string for reproducibility
    QString dataSourceId;
    QString dataRefreshedAt;    // UTC ISO 8601 — when HistoricalDataManager last fetched
    QString createdAt;          // UTC ISO 8601

    // --- canonical scope fields ---
    QString strategyDefId;      // canonical definition UUID
    QString scopeType = QStringLiteral("strategy");  // "strategy" | "portfolio" | "account"
    QString scopeRefId;         // UUID of scope object (see truth table in design)
    int     strategyVersion = 1; // definition version snapshot at time of run

    // v3 catalog fields
    QString catalogStrategyId;  // FK → strategies.strategy_id
    QString catalogVersionId;   // FK → strategy_versions.version_id
};

// ---------------------------------------------------------------------------
// BacktestLoadedRun — composed in-memory result, assembled after run or from DB
// ---------------------------------------------------------------------------
struct BacktestLoadedRun {
    BacktestRunRecord record;

    // BacktestResult carries: totalReturn, sharpe, maxDrawdown, winRate, totalTrades,
    // initialCapital, finalCapital, annualizedReturn, alphaVsBenchmark,
    // tradeLog (QVector<FilledOrder>), equityCurve (QVector<LedgerSnapshot>),
    // benchmark (BenchmarkResult with its own equityCurve).
    BacktestResult result;

    // OHLC bars per symbol — fetched by HistoricalDataManager during the run.
    // Populated only when result comes from a live run (not from a DB reload).
    // Used to populate the Candlestick chart without a second DB query.
    QMap<QString, QList<DbHistoricalBar>> histBars;

    bool isValid() const { return !record.runId.isEmpty(); }
};

} // namespace Backtest

Q_DECLARE_METATYPE(Backtest::BacktestLoadedRun)

#endif // BACKTEST_BACKTESTDATATYPES_H
