#ifndef BACKTEST_BACKTESTSESSION_H
#define BACKTEST_BACKTESTSESSION_H

#include <QObject>
#include <atomic>
#include <memory>
#include <QJsonObject>
#include <QMap>
#include <QVector>
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/IHistoricalDataSource.h"
#include "Backtest/MarketPriceStore.h"
#include "Backtest/BacktestMarketDataAccessor.h"
#include "Backtest/BacktestHistoricalReadAdapter.h"
#include "Pipeline/NoOpSubscriptionPort.h"
#include "Backtest/SimulatedExecutionAdapter.h"
#include "Backtest/SimulatedLedger.h"
#include "Backtest/BacktestMetricsCollector.h"
#include "Backtest/BenchmarkComparison.h"
#include "Common/IClock.h"
#include "Replay/MarketDataReplayer.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/HistoricalReadPolicy.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"

class QNetworkAccessManager;

namespace Backtest {

class HistoricalDataManager;

// Orchestrator for a single strategy-level backtest session.
// Owns all backtest-specific objects and drives the replay loop.
//
// The pipeline runs in the same thread as the replay loop (no QThread).
// All signal connections are Qt::DirectConnection for deterministic ordering.
//
// Usage (from a worker thread):
//   BacktestSession session(config);
//   connect(&session, &BacktestSession::finished, handler, &Handler::onResult);
//   session.run();  // blocks until done or cancelled
class BacktestSession : public QObject {
    Q_OBJECT
public:
    explicit BacktestSession(const BacktestConfig& config, QObject* parent = nullptr);

    // Optional: inject a pipeline config JSON directly instead of loading from file.
    // Must be called before run(). Takes precedence over config.strategyConfigPath.
    void setPipelineConfig(const QJsonObject& config) { m_inlinePipelineConfig = config; }

    // Optional: inject pre-fetched bars (from HistoricalDataManager) so the session
    // skips its own network fetch. Key = symbol, value = ordered bars.
    // When set, loadHistoricalData() is bypassed entirely.
    void setPreloadedBars(const QMap<QString, QVector<IBComm::HistoricalBar>>& bars) {
        m_preloadedBars = bars;
    }

    // Optional: inject pre-fetched benchmark bars to skip benchmark network fetch.
    void setPreloadedBenchmarkBars(const QVector<IBComm::HistoricalBar>& bars) {
        m_preloadedBenchmarkBars = bars;
    }

    // Access the preloaded strategy bars (for controller to forward to UI after run)
    const QMap<QString, QVector<IBComm::HistoricalBar>>& preloadedBars() const {
        return m_preloadedBars;
    }

    // Optional: inject QNetworkAccessManager for Yahoo Finance (tests, custom proxies).
    // Must be called before run(). Session does not take ownership.
    void setYahooNetworkAccessManager(QNetworkAccessManager* nam) {
        m_yahooNetworkManager = nam;
    }

    /// Optional: same worker-thread `HistoricalDataManager` used for cache-backed `IHistoricalRead`
    /// (semantic alphas). Set before `run()` — typically non-null when `BacktestController` prefetches
    /// Yahoo bars; cleared automatically when the manager is destroyed after `run()`.
    void setHistoricalDataManager(HistoricalDataManager* mgr) { m_historicalDataManager = mgr; }

    /// Set the historical read policy for this session (parsed once by BacktestController).
    /// Must be called before run().
    void setHistoricalReadPolicy(Pipeline::HistoricalReadPolicy policy) {
        m_historicalReadPolicy = policy;
    }

    // Preload-then-replay: all historical data is loaded into MarketDataReplayer
    // before the replay loop starts. Blocks until finished or cancelled.
    void run();
    /// Thread-safe: may be called from any thread (only flips the cancel flag).
    void cancel();

    const BacktestResult& result() const { return m_result; }

signals:
    void progressChanged(int percent);
    void finished(const Backtest::BacktestResult& result);
    void failed(const QString& reason);

private:
    void buildObjectGraph();
    void loadHistoricalData();
    void loadBenchmarkData();
    void driveReplayLoop();

    BacktestConfig                                  m_config;
    std::unique_ptr<IHistoricalDataSource>          m_dataSource;
    std::unique_ptr<MarketDataReplayer>             m_replayer;
    std::unique_ptr<MarketPriceStore>               m_priceStore;
    std::unique_ptr<BacktestMarketDataAccessor>     m_marketDataAccessor;
    std::unique_ptr<BacktestHistoricalReadAdapter>  m_historicalReadAdapter;
    std::unique_ptr<Pipeline::NoOpSubscriptionPort> m_noOpSubscriptionPort;
    HistoricalDataManager*                          m_historicalDataManager = nullptr;
    std::unique_ptr<SimulatedClock>                 m_clock;
    std::unique_ptr<SimulatedExecutionAdapter>      m_execAdapter;
    std::unique_ptr<SimulatedLedger>                m_ledger;
    std::unique_ptr<BacktestMetricsCollector>       m_metrics;
    // Adapter owns the pipeline runner in pure-backtest mode (Phase 10).
    // m_pipelineRunner is a non-owning pointer obtained from the adapter after start().
    std::unique_ptr<CPipelineStrategyAdapter>       m_strategyAdapter;
    Pipeline::StrategyPipelineRunner*               m_pipelineRunner = nullptr;
    BacktestResult                                  m_result;
    std::atomic<bool>                               m_cancelled{false};
    // Set when buildObjectGraph() emitted failed() — run() must not load data or replay.
    bool                                            m_buildFailed = false;
    // Separate from m_cancelled: set only when loadHistoricalData() emits failed().
    // Prevents run() from emitting a second, generic failed() after the specific error
    // has already been emitted inside loadHistoricalData().
    bool                                            m_loadFailed = false;
    QJsonObject                                     m_inlinePipelineConfig;
    QMap<QString, QVector<IBComm::HistoricalBar>>   m_preloadedBars;
    QVector<IBComm::HistoricalBar>                  m_preloadedBenchmarkBars;
    QNetworkAccessManager*                         m_yahooNetworkManager = nullptr;
    Pipeline::HistoricalReadPolicy                  m_historicalReadPolicy = Pipeline::HistoricalReadPolicy::PreferCache;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTSESSION_H
