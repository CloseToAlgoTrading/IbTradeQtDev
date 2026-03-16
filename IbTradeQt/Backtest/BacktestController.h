#ifndef BACKTEST_BACKTESTCONTROLLER_H
#define BACKTEST_BACKTESTCONTROLLER_H

// BacktestController — lifecycle orchestrator for a single backtest run.
//
// Owned by CPresenter (not the dock). Separates run lifecycle from UI.
//
// Lifecycle state machine persisted to DB at every transition:
//   Created → Running → Finished
//                    ↘ Failed
//
// Cooperative cancellation is deferred to v2 — no Cancelled status.
//
// Usage (from CPresenter):
//   auto* ctrl = new BacktestController(dbConnName, networkMgr, this);
//   connect(ctrl, &BacktestController::progressChanged, dock, &Dock::setProgress);
//   connect(ctrl, &BacktestController::finished,        presenter, &CPresenter::onBacktestFinished);
//   connect(ctrl, &BacktestController::failed,          presenter, &CPresenter::onBacktestFailed);
//   ctrl->start(runConfig);

#include <QObject>
#include <QThread>
#include <QElapsedTimer>
#include <QMap>
#include <QList>
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/BacktestSession.h"
#include "DB/dbdatatypes.h"

class QNetworkAccessManager;
class DBHandler;

namespace Backtest {

class HistoricalDataManager;

class BacktestController : public QObject {
    Q_OBJECT

public:
    explicit BacktestController(const QString& dbFileName,
                                QNetworkAccessManager* networkManager = nullptr,
                                QObject* parent = nullptr);
    ~BacktestController() override;

    // Start a new run. Creates the run record in DB with status "Created",
    // then launches HistoricalDataManager + BacktestSession on a background thread.
    // Emits progressChanged / finished / failed during execution.
    void start(const BacktestRunConfig& config);

    // Returns true if a run is currently in progress.
    bool isRunning() const { return m_workerThread && m_workerThread->isRunning(); }

    const QString& currentRunId() const { return m_currentRunId; }
    const QString& dbConnectionName() const { return m_dbConnectionName; }

signals:
    void progressChanged(int percent);
    void statusChanged(const QString& status);
    void finished(const Backtest::BacktestLoadedRun& result);
    void failed(const QString& reason);

private slots:
    void onSessionProgress(int percent);
    void onSessionFinished(const Backtest::BacktestResult& result);
    void onSessionFailed(const QString& reason);

private:
    void persistRunRecord(const DbBacktestRun& run);
    void updateRunStatus(const QString& status,
                         const QString& errorText,
                         qint64 durationMs,
                         const QString& dataRefreshedAt);
    void persistResult(const BacktestResult& result, const QString& dataRefreshedAt);

    static BacktestConfig buildBacktestConfig(const BacktestRunConfig& rc);
    static QString engineVersion();
    static QString downsample(const QVector<LedgerSnapshot>& curve, int maxPoints);

    QString                   m_dbConnectionName;
    QNetworkAccessManager*    m_networkManager = nullptr;
    QString                   m_currentRunId;
    BacktestRunConfig         m_currentConfig;
    QString                   m_dataRefreshedAt;
    QElapsedTimer             m_elapsed;
    // Bars fetched by HistoricalDataManager, forwarded to the UI after the run
    QMap<QString, QList<DbHistoricalBar>> m_lastHistBars;

    QThread*                  m_workerThread  = nullptr;
    BacktestSession*          m_session       = nullptr;
};

} // namespace Backtest

#endif // BACKTEST_BACKTESTCONTROLLER_H
