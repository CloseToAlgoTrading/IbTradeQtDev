#include "BacktestUI/BacktestRunConfigPanel.h"
#include "BacktestConstants.h"
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDate>

namespace BacktestUI {

BacktestRunConfigPanel::BacktestRunConfigPanel(QWidget* parent)
    : QWidget(parent)
{
    buildForm();
}

void BacktestRunConfigPanel::buildForm() {
    // --- Fields ---
    m_symbolsEdit = new QLineEdit(QStringLiteral("AMD,NVDA"));
    m_symbolsEdit->setPlaceholderText(QStringLiteral("Comma-separated, e.g. AMD,NVDA"));

    m_startDateEdit = new QDateEdit(QDate::currentDate().addYears(-5));
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));

    m_endDateEdit = new QDateEdit(QDate::currentDate());
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));

    m_capitalSpin = new QDoubleSpinBox();
    m_capitalSpin->setRange(1000.0, 100'000'000.0);
    m_capitalSpin->setValue(100'000.0);
    m_capitalSpin->setSingleStep(10'000.0);
    m_capitalSpin->setPrefix(QStringLiteral("$ "));
    m_capitalSpin->setDecimals(0);

    m_benchmarkEdit = new QLineEdit(QStringLiteral("SPY"));
    m_benchmarkEdit->setPlaceholderText(QStringLiteral("Optional, e.g. SPY"));

    m_resolutionCombo = new QComboBox();
    m_resolutionCombo->addItems({"Day1", "Hour1", "Min30", "Min15", "Min5", "Min1"});

    m_fillModelCombo = new QComboBox();
    m_fillModelCombo->addItems({"BidAsk", "MidPrice", "Instant", "SlippageBps"});

    m_fillTimingCombo = new QComboBox();
    m_fillTimingCombo->addItems({
        "SignalOnClose_FillNextBarOpen",
        "SignalOnClose_FillAtClose",
        "SignalOnTick_FillAtBidAsk"
    });

    m_slippageSpin = new QDoubleSpinBox();
    m_slippageSpin->setRange(0.0, 100.0);
    m_slippageSpin->setValue(1.0);
    m_slippageSpin->setSingleStep(0.5);
    m_slippageSpin->setSuffix(QStringLiteral(" bps"));
    m_slippageSpin->setDecimals(1);

    m_dataSourceCombo = new QComboBox();
    m_dataSourceCombo->addItems({"yahoo", "csv", "jsonl"});

    // --- Run button & progress ---
    m_runButton = new QPushButton(QStringLiteral("▶  Run Backtest"));
    m_runButton->setMinimumHeight(32);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);

    m_statusLabel = new QLabel(QStringLiteral("Ready"));
    m_statusLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));

    // --- Layout ---
    auto* form = new QFormLayout();
    form->addRow(QStringLiteral("Symbols:"),      m_symbolsEdit);
    form->addRow(QStringLiteral("Start Date:"),   m_startDateEdit);
    form->addRow(QStringLiteral("End Date:"),      m_endDateEdit);
    form->addRow(QStringLiteral("Capital:"),       m_capitalSpin);
    form->addRow(QStringLiteral("Benchmark:"),     m_benchmarkEdit);
    form->addRow(QStringLiteral("Resolution:"),    m_resolutionCombo);
    form->addRow(QStringLiteral("Fill Model:"),    m_fillModelCombo);
    form->addRow(QStringLiteral("Fill Timing:"),   m_fillTimingCombo);
    form->addRow(QStringLiteral("Slippage:"),      m_slippageSpin);
    form->addRow(QStringLiteral("Data Source:"),   m_dataSourceCombo);

    auto* group = new QGroupBox(QStringLiteral("Run Configuration"));
    group->setLayout(form);

    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(m_runButton);
    btnRow->addWidget(m_statusLabel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(group);
    layout->addLayout(btnRow);
    layout->addWidget(m_progressBar);

    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunConfigPanel::onRunClicked);
}

void BacktestRunConfigPanel::applyProfile(const Backtest::BacktestProfile& profile) {
    if (!profile.defaultBenchmark.isEmpty())
        m_benchmarkEdit->setText(profile.defaultBenchmark);

    const int resIdx = m_resolutionCombo->findText(profile.defaultResolution);
    if (resIdx >= 0) m_resolutionCombo->setCurrentIndex(resIdx);

    const int srcIdx = m_dataSourceCombo->findText(profile.defaultDataSource);
    if (srcIdx >= 0) m_dataSourceCombo->setCurrentIndex(srcIdx);
}

void BacktestRunConfigPanel::setStrategyContext(const QString& strategyId,
                                                 const QString& displayName,
                                                 const QString& portfolioPath,
                                                 const QString& pipelineConfigJson,
                                                 const QString& strategyDefId,
                                                 int            strategyVersion) {
    m_strategyId         = strategyId;
    m_displayName        = displayName;
    m_portfolioPath      = portfolioPath;
    m_pipelineConfigJson = pipelineConfigJson;
    m_strategyDefId      = strategyDefId;
    m_strategyVersion    = strategyVersion > 0 ? strategyVersion : 1;
}

Backtest::BacktestRunConfig BacktestRunConfigPanel::currentConfig() const {
    Backtest::BacktestRunConfig c;
    c.strategyId          = m_strategyId;
    c.strategyDisplayName = m_displayName;
    c.portfolioPath       = m_portfolioPath;
    c.pipelineConfigJson  = m_pipelineConfigJson;
    // Canonical strategy definition fields — populated when a live strategy node is
    // selected via "Open in Backtest Workspace". Empty when launched standalone.
    c.strategyDefId       = m_strategyDefId;
    c.strategyVersion     = m_strategyVersion;
    c.scopeType           = QString(Backtest::Scope::Strategy);
    // scopeRefId: use defId when available (preferred); fall back to live node UUID.
    c.scopeRefId          = m_strategyDefId.isEmpty() ? m_strategyId : m_strategyDefId;

    const QString symsText = m_symbolsEdit->text().trimmed();
    for (const QString& s : symsText.split(',', Qt::SkipEmptyParts))
        c.symbols.append(s.trimmed().toUpper());

    c.startDate      = QDateTime(m_startDateEdit->date(), QTime(0, 0), Qt::UTC);
    c.endDate        = QDateTime(m_endDateEdit->date(),   QTime(23, 59, 59), Qt::UTC);
    c.initialCapital = m_capitalSpin->value();
    c.benchmarkSymbol = m_benchmarkEdit->text().trimmed().toUpper();
    c.resolution     = m_resolutionCombo->currentText();
    c.fillModel      = m_fillModelCombo->currentText();
    c.fillTiming     = m_fillTimingCombo->currentText();
    c.slippageBps    = m_slippageSpin->value();
    c.dataSourceId   = m_dataSourceCombo->currentText();

    return c;
}

void BacktestRunConfigPanel::setProgress(int percent) {
    m_progressBar->setValue(percent);
}

void BacktestRunConfigPanel::setStatus(const QString& status) {
    m_statusLabel->setText(status);
}

void BacktestRunConfigPanel::setRunning(bool running) {
    m_runButton->setEnabled(!running);
    m_progressBar->setVisible(running);
    if (!running) m_progressBar->setValue(0);
}

void BacktestRunConfigPanel::onRunClicked() {
    emit runRequested(currentConfig());
}

} // namespace BacktestUI
