#ifndef BACKTEST_BACKTESTSESSION_H
#define BACKTEST_BACKTESTSESSION_H

#include <QObject>
#include <memory>
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/IHistoricalDataSource.h"
#include "Backtest/MarketPriceStore.h"
#include "Backtest/SimulatedExecutionAdapter.h"
#include "Backtest/SimulatedLedger.h"
#include "Backtest/BacktestMetricsCollector.h"
#include "Backtest/BenchmarkComparison.h"
#include "Common/IClock.h"
#include "Replay/MarketDataReplayer.h"
#include "Pipeline/StrategyPipelineRunner.h"

namespace Backtest {

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

    // Preload-then-replay: all historical data is loaded into MarketDataReplayer
    // before the replay loop starts. Blocks until finished or cancelled.
    void run();
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
    std::unique_ptr<SimulatedClock>                 m_clock;
    std::unique_ptr<SimulatedExecutionAdapter>      m_execAdapter;
    std::unique_ptr<SimulatedLedger>                m_ledger;
    std::unique_ptr<BacktestMetricsCollector>       m_metrics;
    std::unique_ptr<Pipeline::StrategyPipelineRunner> m_pipelineRunner;
    BacktestResult                                  m_result;
    bool                                            m_cancelled = false;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTSESSION_H
