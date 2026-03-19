#ifndef RUNTIMEPOLICYEDITOR_H
#define RUNTIMEPOLICYEDITOR_H

#include <QWidget>
#include <QJsonObject>
#include "Pipeline/StrategyRuntimePolicy.h"

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QGroupBox;

class RuntimePolicyEditor : public QWidget
{
    Q_OBJECT

public:
    explicit RuntimePolicyEditor(QWidget* parent = nullptr);

    void setPolicy(const Pipeline::StrategyRuntimePolicy& policy);
    Pipeline::StrategyRuntimePolicy policy() const;

    void setReadOnly(bool readOnly);

    void loadFromJson(const QJsonObject& pipelineConfig);
    QJsonObject applyToJson(const QJsonObject& pipelineConfig) const;

    // Delegates to StrategyRuntimePolicy (single source of truth)
    static QString evalModeLabel(Pipeline::StrategyRuntimePolicy::EvaluationMode mode)
    { return Pipeline::StrategyRuntimePolicy::evalModeLabel(mode); }
    static QString rebalModeLabel(Pipeline::StrategyRuntimePolicy::RebalanceMode mode)
    { return Pipeline::StrategyRuntimePolicy::rebalModeLabel(mode); }
    static QString policySummary(const Pipeline::StrategyRuntimePolicy& p)
    { return p.summary(); }

signals:
    void policyChanged(const Pipeline::StrategyRuntimePolicy& policy);

private:
    void buildUi();
    void updateIntervalVisibility();
    void emitChanged();

    bool m_readOnly = false;
    bool m_blockSignals = false;

    // Evaluation
    QComboBox* m_evalModeCombo     = nullptr;
    QSpinBox*  m_evalIntervalSpin  = nullptr;
    QLabel*    m_evalIntervalLabel = nullptr;

    // Rebalance
    QComboBox* m_rebalModeCombo     = nullptr;
    QSpinBox*  m_rebalIntervalSpin  = nullptr;
    QLabel*    m_rebalIntervalLabel = nullptr;

    // Signals
    QCheckBox* m_accumulateCheck    = nullptr;
    QSpinBox*  m_expiryBarsSpin     = nullptr;
    QLabel*    m_expiryLabel        = nullptr;

    // Risk
    QCheckBox* m_riskAlwaysCheck    = nullptr;
    QCheckBox* m_cancelPendingCheck = nullptr;

    // Execution
    QCheckBox* m_execImmediateCheck = nullptr;
};

#endif // RUNTIMEPOLICYEDITOR_H
