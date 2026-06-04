#include "RuntimePolicyEditor.h"

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>

RuntimePolicyEditor::RuntimePolicyEditor(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    setPolicy(Pipeline::StrategyRuntimePolicy{});
}

void RuntimePolicyEditor::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    // --- Evaluation ---
    auto* evalGroup = new QGroupBox(QStringLiteral("Evaluation Cadence"));
    auto* evalForm  = new QFormLayout(evalGroup);

    m_evalModeCombo = new QComboBox;
    m_evalModeCombo->addItem(QStringLiteral("Every Tick"));
    m_evalModeCombo->addItem(QStringLiteral("Every Bar Close"));
    m_evalModeCombo->addItem(QStringLiteral("Every N Bars"));
    m_evalModeCombo->addItem(QStringLiteral("Every N Minutes"));
    m_evalModeCombo->addItem(QStringLiteral("Every N Days"));
    m_evalModeCombo->setToolTip(
        QStringLiteral("Controls how often alpha and selection logic are evaluated: each tick, each completed bar, or a fixed bar/minute/day interval."));
    evalForm->addRow(QStringLiteral("Mode:"), m_evalModeCombo);

    m_evalIntervalLabel = new QLabel(QStringLiteral("Interval N:"));
    m_evalIntervalSpin  = new QSpinBox;
    m_evalIntervalSpin->setRange(1, 10000);
    m_evalIntervalSpin->setValue(1);
    m_evalIntervalSpin->setToolTip(
        QStringLiteral("Interval count used when the evaluation mode is Every N Bars, Every N Minutes, or Every N Days."));
    evalForm->addRow(m_evalIntervalLabel, m_evalIntervalSpin);

    root->addWidget(evalGroup);

    // --- Rebalance ---
    auto* rebalGroup = new QGroupBox(QStringLiteral("Rebalance Cadence"));
    auto* rebalForm  = new QFormLayout(rebalGroup);

    m_rebalModeCombo = new QComboBox;
    m_rebalModeCombo->addItem(QStringLiteral("Immediate"));
    m_rebalModeCombo->addItem(QStringLiteral("Every N Bars"));
    m_rebalModeCombo->addItem(QStringLiteral("Every N Minutes"));
    m_rebalModeCombo->addItem(QStringLiteral("Every N Days"));
    m_rebalModeCombo->setToolTip(
        QStringLiteral("Controls how often approved target positions are turned into rebalance actions."));
    rebalForm->addRow(QStringLiteral("Mode:"), m_rebalModeCombo);

    m_rebalIntervalLabel = new QLabel(QStringLiteral("Interval N:"));
    m_rebalIntervalSpin  = new QSpinBox;
    m_rebalIntervalSpin->setRange(1, 10000);
    m_rebalIntervalSpin->setValue(1);
    m_rebalIntervalSpin->setToolTip(
        QStringLiteral("Interval count used when the rebalance mode is Every N Bars, Every N Minutes, or Every N Days."));
    rebalForm->addRow(m_rebalIntervalLabel, m_rebalIntervalSpin);

    root->addWidget(rebalGroup);

    // --- Signal Accumulation ---
    auto* sigGroup = new QGroupBox(QStringLiteral("Signal Accumulation"));
    auto* sigForm  = new QFormLayout(sigGroup);

    m_accumulateCheck = new QCheckBox(QStringLiteral("Accumulate alpha signals between rebalance windows"));
    m_accumulateCheck->setToolTip(
        QStringLiteral("When enabled, alpha signals are retained between rebalance windows so the next rebalance can act on accumulated signals."));
    sigForm->addRow(m_accumulateCheck);

    m_expiryLabel   = new QLabel(QStringLiteral("Signal Expiry (bars):"));
    m_expiryBarsSpin = new QSpinBox;
    m_expiryBarsSpin->setRange(0, 10000);
    m_expiryBarsSpin->setValue(0);
    m_expiryBarsSpin->setSpecialValueText(QStringLiteral("Never"));
    m_expiryBarsSpin->setToolTip(
        QStringLiteral("Maximum number of bars an accumulated alpha signal remains valid. Use Never to keep signals until replaced or consumed."));
    sigForm->addRow(m_expiryLabel, m_expiryBarsSpin);

    root->addWidget(sigGroup);

    // --- Risk Override ---
    auto* riskGroup = new QGroupBox(QStringLiteral("Risk Override"));
    auto* riskForm  = new QFormLayout(riskGroup);

    m_riskAlwaysCheck    = new QCheckBox(QStringLiteral("Risk always active (tick-by-tick monitoring)"));
    m_cancelPendingCheck = new QCheckBox(QStringLiteral("Cancel pending orders on emergency risk"));
    m_riskAlwaysCheck->setToolTip(
        QStringLiteral("Runs risk checks continuously instead of only during scheduled rebalance windows."));
    m_cancelPendingCheck->setToolTip(
        QStringLiteral("Allows emergency risk checks to cancel pending orders when the strategy breaches configured risk conditions."));
    riskForm->addRow(m_riskAlwaysCheck);
    riskForm->addRow(m_cancelPendingCheck);

    root->addWidget(riskGroup);

    // --- Execution ---
    auto* execGroup = new QGroupBox(QStringLiteral("Execution"));
    auto* execForm  = new QFormLayout(execGroup);

    m_execImmediateCheck = new QCheckBox(QStringLiteral("Execute immediately after risk approval"));
    m_execImmediateCheck->setToolTip(
        QStringLiteral("Sends approved orders to the execution layer immediately after risk approval instead of waiting for the next execution cycle."));
    execForm->addRow(m_execImmediateCheck);

    root->addWidget(execGroup);

    root->addStretch();

    // --- Connections ---
    connect(m_evalModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { updateIntervalVisibility(); emitChanged(); });
    connect(m_evalIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emitChanged(); });
    connect(m_rebalModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { updateIntervalVisibility(); emitChanged(); });
    connect(m_rebalIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emitChanged(); });
    connect(m_accumulateCheck, &QCheckBox::toggled, this, [this](bool on) {
        m_expiryBarsSpin->setEnabled(on && !m_readOnly);
        m_expiryLabel->setEnabled(on && !m_readOnly);
        emitChanged();
    });
    connect(m_expiryBarsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emitChanged(); });
    connect(m_riskAlwaysCheck, &QCheckBox::toggled,
            this, [this]() { emitChanged(); });
    connect(m_cancelPendingCheck, &QCheckBox::toggled,
            this, [this]() { emitChanged(); });
    connect(m_execImmediateCheck, &QCheckBox::toggled,
            this, [this]() { emitChanged(); });

    updateIntervalVisibility();
}

void RuntimePolicyEditor::setPolicy(const Pipeline::StrategyRuntimePolicy& p)
{
    m_blockSignals = true;

    m_evalModeCombo->setCurrentIndex(static_cast<int>(p.evaluationMode));
    m_evalIntervalSpin->setValue(p.evaluationIntervalN);

    m_rebalModeCombo->setCurrentIndex(static_cast<int>(p.rebalanceMode));
    m_rebalIntervalSpin->setValue(p.rebalanceIntervalN);

    m_accumulateCheck->setChecked(p.accumulateAlphaSignals);
    m_expiryBarsSpin->setValue(p.signalExpiryBars);

    m_riskAlwaysCheck->setChecked(p.riskAlwaysActive);
    m_cancelPendingCheck->setChecked(p.riskCanCancelPendingOrders);

    m_execImmediateCheck->setChecked(p.executionImmediateAfterApproval);

    updateIntervalVisibility();
    m_blockSignals = false;
}

Pipeline::StrategyRuntimePolicy RuntimePolicyEditor::policy() const
{
    Pipeline::StrategyRuntimePolicy p;

    p.evaluationMode = static_cast<Pipeline::StrategyRuntimePolicy::EvaluationMode>(
        m_evalModeCombo->currentIndex());
    p.evaluationIntervalN = m_evalIntervalSpin->value();

    p.rebalanceMode = static_cast<Pipeline::StrategyRuntimePolicy::RebalanceMode>(
        m_rebalModeCombo->currentIndex());
    p.rebalanceIntervalN = m_rebalIntervalSpin->value();

    p.accumulateAlphaSignals = m_accumulateCheck->isChecked();
    p.signalExpiryBars = m_expiryBarsSpin->value();

    p.riskAlwaysActive = m_riskAlwaysCheck->isChecked();
    p.riskCanCancelPendingOrders = m_cancelPendingCheck->isChecked();

    p.executionImmediateAfterApproval = m_execImmediateCheck->isChecked();

    return p;
}

void RuntimePolicyEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    m_evalModeCombo->setEnabled(!readOnly);
    m_evalIntervalSpin->setEnabled(!readOnly);
    m_rebalModeCombo->setEnabled(!readOnly);
    m_rebalIntervalSpin->setEnabled(!readOnly);
    m_accumulateCheck->setEnabled(!readOnly);
    m_expiryBarsSpin->setEnabled(!readOnly && m_accumulateCheck->isChecked());
    m_riskAlwaysCheck->setEnabled(!readOnly);
    m_cancelPendingCheck->setEnabled(!readOnly);
    m_execImmediateCheck->setEnabled(!readOnly);
    updateIntervalVisibility();
}

void RuntimePolicyEditor::loadFromJson(const QJsonObject& pipelineConfig)
{
    QJsonObject policyObj = pipelineConfig.value("runtimePolicy").toObject();
    setPolicy(Pipeline::StrategyRuntimePolicy::fromJson(policyObj));
}

QJsonObject RuntimePolicyEditor::applyToJson(const QJsonObject& pipelineConfig) const
{
    QJsonObject result = pipelineConfig;
    result["runtimePolicy"] = policy().toJson();
    return result;
}

void RuntimePolicyEditor::updateIntervalVisibility()
{
    // EveryTick (0) and EveryBarClose (1) don't need an interval N
    int evalIdx = m_evalModeCombo->currentIndex();
    bool evalNeedsInterval = (evalIdx >= 2);
    m_evalIntervalSpin->setVisible(evalNeedsInterval);
    m_evalIntervalLabel->setVisible(evalNeedsInterval);

    // Immediate (0) doesn't need an interval N
    bool rebalNeedsInterval = (m_rebalModeCombo->currentIndex() != 0);
    m_rebalIntervalSpin->setVisible(rebalNeedsInterval);
    m_rebalIntervalLabel->setVisible(rebalNeedsInterval);
}

void RuntimePolicyEditor::emitChanged()
{
    if (!m_blockSignals)
        emit policyChanged(policy());
}
