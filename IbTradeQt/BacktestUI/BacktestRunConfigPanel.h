#ifndef BACKTESTUI_BACKTESTRUNCONFIPANEL_H
#define BACKTESTUI_BACKTESTRUNCONFIPANEL_H

// BacktestRunConfigPanel — form widget for configuring a single backtest run.
//
// All fields map directly to BacktestRunConfig. The form is pre-populated
// from BacktestProfile defaults when a strategy is selected. Fields:
//   Symbols, Start Date, End Date, Initial Capital, Benchmark,
//   Resolution, Fill Model, Fill Timing, Slippage (bps), Data Source.
//
// Run button emits runRequested(BacktestRunConfig) for CPresenter to handle.
// A QProgressBar shows run progress (0–100%). A status label shows lifecycle state.

#include <QWidget>
#include "Backtest/BacktestDataTypes.h"

class QLineEdit;
class QDateEdit;
class QDoubleSpinBox;
class QComboBox;
class QProgressBar;
class QLabel;
class QPushButton;

namespace BacktestUI {

class BacktestRunConfigPanel : public QWidget {
    Q_OBJECT

public:
    explicit BacktestRunConfigPanel(QWidget* parent = nullptr);

    // Pre-populate from strategy-level defaults.
    void applyProfile(const Backtest::BacktestProfile& profile);

    // Build a BacktestRunConfig from current form values.
    // strategyId / displayName / portfolioPath / pipelineConfigJson must be
    // set externally via setStrategyContext() before calling this.
    Backtest::BacktestRunConfig currentConfig() const;

    // Called by BacktestWorkspaceDock after strategy selection.
    // strategyDefId and strategyVersion are set from the strategy catalog;
    // they will be propagated into BacktestRunConfig when the user clicks Run.
    void setStrategyContext(const QString& strategyId,
                            const QString& displayName,
                            const QString& portfolioPath,
                            const QString& pipelineConfigJson,
                            const QString& strategyDefId   = {},
                            int            strategyVersion = 1);

    void setCatalogVersionId(const QString& versionId) { m_catalogVersionId = versionId; }

    // Progress / status (called by BacktestWorkspaceDock)
    void setProgress(int percent);
    void setStatus(const QString& status);
    void setRunning(bool running);

signals:
    void runRequested(const Backtest::BacktestRunConfig& config);

private slots:
    void onRunClicked();

private:
    void buildForm();

    // Strategy context (set from outside, not editable in the form)
    QString m_strategyId;
    QString m_displayName;
    QString m_portfolioPath;
    QString m_pipelineConfigJson;
    QString m_strategyDefId;
    QString m_catalogVersionId;
    int     m_strategyVersion = 1;

    // Form fields
    QLineEdit*      m_symbolsEdit      = nullptr;
    QDateEdit*      m_startDateEdit    = nullptr;
    QDateEdit*      m_endDateEdit      = nullptr;
    QDoubleSpinBox* m_capitalSpin      = nullptr;
    QLineEdit*      m_benchmarkEdit    = nullptr;
    QComboBox*      m_resolutionCombo  = nullptr;
    QComboBox*      m_fillModelCombo   = nullptr;
    QComboBox*      m_fillTimingCombo  = nullptr;
    QDoubleSpinBox* m_slippageSpin     = nullptr;
    QComboBox*      m_dataSourceCombo  = nullptr;

    QPushButton*    m_runButton        = nullptr;
    QProgressBar*   m_progressBar      = nullptr;
    QLabel*         m_statusLabel      = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTRUNCONFIPANEL_H
