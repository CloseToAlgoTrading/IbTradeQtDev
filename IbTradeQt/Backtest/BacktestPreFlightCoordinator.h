#ifndef BACKTEST_BACKTESTPREFLIGHTCOORDINATOR_H
#define BACKTEST_BACKTESTPREFLIGHTCOORDINATOR_H

#include "Backtest/BacktestDataTypes.h"
#include "Backtest/HistoricalDataManager.h"
#include "Backtest/YahooUniverseValidator.h"
#include "Pipeline/UniverseResolver.h"

#include <QHash>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

namespace Backtest {

/// Result of a full "Prepare run" check (Yahoo validation + DB coverage). No network in coverage fields.
struct BacktestPreFlightResult {
    bool ok = false;
    QString errorMessage;

    Pipeline::UniverseResolutionResult::Mode universeMode =
        Pipeline::UniverseResolutionResult::Mode::Unsupported;
    QString universeDetail;

    QStringList resolvedSymbols;

    YahooUniverseValidator::Result yahooValidation;

    QMap<QString, SymbolCoveragePlanEntry> strategySymbolCoverage;
    bool hasBenchmarkCoverage = false;
    QString benchmarkSymbolForCoverage;
    SymbolCoveragePlanEntry benchmarkCoverageEntry;

    /// True when Prepare should offer follow-up data actions (Yahoo failures or incomplete cache coverage).
    bool needsDataAttention() const;
};

/// Sequences universe resolution → Yahoo validation (when applicable) → read-only coverage. No UI logic.
class BacktestPreFlightCoordinator : public QObject {
    Q_OBJECT

public:
    struct PrepareInput {
        QString              dbFilePath;
        BacktestRunConfig    runConfig;
        HistoricalDataManager::Config hdmConfig;
        YahooUniverseValidator::Config yahooValidatorConfig;
        /// Required for Yahoo validation and optional for DB; if null, Yahoo steps are skipped where possible.
        QNetworkAccessManager* networkAccessManager = nullptr;
    };

    explicit BacktestPreFlightCoordinator(QObject* parent = nullptr);

    /// Same symbol resolution as `BacktestController::start` (pipeline static universe when symbols empty).
    static BacktestRunConfig resolveRunConfigSymbols(const BacktestRunConfig& config);

    /// Resolved universe minus Yahoo validation failures; clears benchmark if it failed. Mutates \a cfg in place.
    static QStringList stripRunConfigRemovingYahooFailures(
        BacktestRunConfig* cfg,
        const QHash<QString, QString>& failedBySymbol);

    /// Runs on the caller thread: opens a temporary SQLite connection, performs work, closes it.
    /// For Yahoo data source, pass a QNetworkAccessManager bound to this thread (tests: mock on main thread).
    static BacktestPreFlightResult runPrepareSync(const PrepareInput& input);

    void requestPrepareAsync(const PrepareInput& input);

    /// Lightweight invalid-symbol probe for Run (Yahoo only). Executes on a background thread; creates NAM there.
    void requestYahooSymbolCheckAsync(const QStringList& symbols);

signals:
    void prepareFinished(const Backtest::BacktestPreFlightResult& result);
    void yahooSymbolCheckFinished(const YahooUniverseValidator::Result& result);

private:
    static QHash<QString, QVariantMap> assetListJsonToHash(const QString& json);
};

} // namespace Backtest

#endif
