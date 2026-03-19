#include "StrategyWorkspace.h"
#include "LayoutConstants.h"
#include "MetricsStrip.h"
#include "WorkspaceHeader.h"
#include "BlockInspectorPanel.h"
#include "RuntimePolicyEditor.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include "cpipelinestrategyadapter.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include "Pipeline/UniverseResolver.h"
#include "Pipeline/StrategyRuntimePolicy.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTabWidget>
#include <QScrollArea>
#include <QJsonArray>

StrategyWorkspace::StrategyWorkspace(QWidget* parent)
    : WorkspaceBase(parent)
{
    buildOverviewTab();
    buildPropertiesTab();
    buildAssetsTab();
    buildPolicyTab();
    buildInfoTab();
    buildLogsTab();
}

// ---- Tab construction ----

void StrategyWorkspace::buildOverviewTab()
{
    m_overviewWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_overviewWidget);
    layout->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                               Layout::TabContentMargin, Layout::TabContentMargin);
    layout->setSpacing(Layout::SectionSpacing);

    m_ovStateSummary = new QLabel("--", m_overviewWidget);
    m_ovCurrentPos   = new QLabel("Positions: --", m_overviewWidget);
    m_ovLatestSignal = new QLabel("Last signal: --", m_overviewWidget);
    m_ovWarnings     = new QLabel("", m_overviewWidget);
    m_ovWarnings->setStyleSheet("color: #eab308;");

    m_ovEvalMode  = new QLabel("Evaluation: --", m_overviewWidget);
    m_ovEvalMode->setStyleSheet("color: #888; font-size: 12px;");
    m_ovRebalMode = new QLabel("Rebalance: --", m_overviewWidget);
    m_ovRebalMode->setStyleSheet("color: #888; font-size: 12px;");

    layout->addWidget(new QLabel("<b>State</b>", m_overviewWidget));
    layout->addWidget(m_ovStateSummary);
    layout->addWidget(new QLabel("<b>Current Position</b>", m_overviewWidget));
    layout->addWidget(m_ovCurrentPos);
    layout->addWidget(new QLabel("<b>Latest Signal</b>", m_overviewWidget));
    layout->addWidget(m_ovLatestSignal);
    layout->addWidget(new QLabel("<b>Runtime Policy</b>", m_overviewWidget));
    layout->addWidget(m_ovEvalMode);
    layout->addWidget(m_ovRebalMode);
    layout->addWidget(m_ovWarnings);
    layout->addStretch();

    addTab("Overview", m_overviewWidget);
}

void StrategyWorkspace::buildPropertiesTab()
{
    auto* stack = new QStackedWidget(this);

    auto* scroll = new QScrollArea(stack);
    scroll->setWidgetResizable(true);
    m_propertiesWidget = new QWidget(scroll);
    m_propertiesForm = new QFormLayout(m_propertiesWidget);
    m_propertiesForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                          Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_propertiesWidget);
    m_propertiesScroll = scroll;

    m_inspector = new BlockInspectorPanel(stack);
    m_inspector->setReadOnly(true);

    stack->addWidget(scroll);
    stack->addWidget(m_inspector);
    stack->setCurrentWidget(scroll);

    addTab("Properties", stack);
}

void StrategyWorkspace::buildInfoTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_infoWidget = new QWidget(scroll);
    m_infoForm = new QFormLayout(m_infoWidget);
    m_infoForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                    Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_infoWidget);
    addTab("Info", scroll);
}

void StrategyWorkspace::buildLogsTab()
{
    m_logsWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_logsWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_logsText = new QTextEdit(m_logsWidget);
    m_logsText->setReadOnly(true);
    layout->addWidget(m_logsText);
    addTab("Logs", m_logsWidget);
}

void StrategyWorkspace::buildAssetsTab()
{
    m_assetsWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_assetsWidget);
    layout->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                               Layout::TabContentMargin, Layout::TabContentMargin);
    layout->setSpacing(Layout::SectionSpacing);

    auto* hint = new QLabel("Symbols configured in the selection block(s) of this strategy's pipeline.",
                             m_assetsWidget);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #888; font-size: 11px;");
    layout->addWidget(hint);

    layout->addWidget(new QLabel("<b>Asset Universe</b>", m_assetsWidget));

    m_assetsEdit = new QLineEdit(m_assetsWidget);
    m_assetsEdit->setPlaceholderText("e.g. AAPL, MSFT, GOOG");
    connect(m_assetsEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_boundModel) return;
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
        if (!adapter) return;

        QStringList symbols;
        for (const auto& s : m_assetsEdit->text().split(','))
            if (!s.trimmed().isEmpty())
                symbols.append(s.trimmed());

        QJsonArray symArr;
        for (const auto& s : symbols) symArr.append(s);

        QJsonObject cfg = adapter->pipelineConfig();
        QJsonValue selVal = cfg.value("selection");

        if (selVal.isArray()) {
            QJsonArray arr = selVal.toArray();
            if (!arr.isEmpty()) {
                QJsonObject entry = arr[0].toObject();
                QJsonObject blockCfg = entry.value("config").toObject();
                blockCfg["symbols"] = symArr;
                entry["config"] = blockCfg;
                arr[0] = entry;
                cfg["selection"] = arr;
            }
        } else if (selVal.isObject()) {
            QJsonObject entry = selVal.toObject();
            QJsonObject blockCfg = entry.value("config").toObject();
            blockCfg["symbols"] = symArr;
            entry["config"] = blockCfg;
            cfg["selection"] = entry;
        }

        adapter->setPipelineConfig(cfg);
    });
    layout->addWidget(m_assetsEdit);

    m_universeInfoLabel = new QLabel(m_assetsWidget);
    m_universeInfoLabel->setWordWrap(true);
    m_universeInfoLabel->setStyleSheet("color: #888; font-size: 11px; font-style: italic;");
    layout->addWidget(m_universeInfoLabel);

    layout->addStretch();

    addTab("Assets", m_assetsWidget);
}

void StrategyWorkspace::buildPolicyTab()
{
    m_policyEditor = new RuntimePolicyEditor(this);
    m_policyEditor->setReadOnly(true);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_policyEditor);

    addTab("Policy", scroll);
}

// ---- Public API ----

void StrategyWorkspace::showBlockInProperties(const QString& category,
                                               const QString& jsonKey,
                                               bool isArray, int arrayIndex)
{
    if (!m_inspector || !m_boundModel) return;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) return;

    m_inspector->showBlock(adapter->pipelineConfig(),
                           category, jsonKey, isArray, arrayIndex);

    auto* stack = qobject_cast<QStackedWidget*>(m_inspector->parentWidget());
    if (stack) stack->setCurrentWidget(m_inspector);
    m_showingBlock = true;

    int propertiesIdx = m_tabWidget->indexOf(stack);
    if (propertiesIdx >= 0)
        m_tabWidget->setCurrentIndex(propertiesIdx);
}

void StrategyWorkspace::restoreStrategyProperties()
{
    if (!m_showingBlock) return;

    auto* stack = qobject_cast<QStackedWidget*>(m_inspector->parentWidget());
    if (stack) stack->setCurrentWidget(m_propertiesScroll);
    m_showingBlock = false;
    m_inspector->clear();
}

// ---- Lifecycle ----

void StrategyWorkspace::onContextSet()
{
    auto* baseModel = dynamic_cast<CBaseModel*>(m_boundModel);
    if (baseModel) {
        m_connections.append(
            connect(baseModel, &CBaseModel::displayStateChanged, this, [this](DisplayState, DisplayState newState) {
                m_header->setState(newState);
                refreshOverview();
            }));
    }

    QString breadcrumb;
    if (m_parentModel)
        breadcrumb = m_parentModel->getName();

    DisplayState ds = DisplayState::Idle;
    if (baseModel) ds = baseModel->resolveDisplayState();

    setHeaderInfo(m_boundModel->getName(), breadcrumb, ds);
    refreshProperties();
    refreshAssets();
    refreshPolicy();
    restoreStrategyProperties();
    m_tabWidget->setCurrentIndex(0);
}

void StrategyWorkspace::onContextCleared()
{
    m_header->clear();
    m_metricsStrip->clear();
    m_ovStateSummary->setText("--");
    m_ovCurrentPos->setText("Positions: --");
    m_ovLatestSignal->setText("Last signal: --");
    m_ovEvalMode->setText("Evaluation: --");
    m_ovRebalMode->setText("Rebalance: --");
    m_ovWarnings->clear();
    m_logsText->clear();

    while (m_propertiesForm->rowCount() > 0)
        m_propertiesForm->removeRow(0);

    while (m_infoForm->rowCount() > 0)
        m_infoForm->removeRow(0);

    if (m_assetsEdit) m_assetsEdit->clear();
    restoreStrategyProperties();
}

// ---- Data refresh ----

void StrategyWorkspace::refreshMetrics()
{
    if (!m_boundModel) return;

    QVariantMap info = m_boundModel->genericInfo();

    QList<MetricCard> cards;
    auto addCard = [&](const QString& label, const QString& key, const QColor& posColor = {}) {
        double val = info.value(key).toDouble();
        QColor c;
        if (posColor.isValid()) {
            c = (val >= 0) ? QColor(34, 197, 94) : QColor(239, 68, 68);
        }
        cards.append({label, QString::number(val, 'f', 2), c, key});
    };

    addCard("PnL", MandatoryInfo::Strategy::DailyPnL, QColor(1,1,1));
    addCard("Unrealized", MandatoryInfo::Strategy::UnrealizedPnL, QColor(1,1,1));
    addCard("Realized", MandatoryInfo::Strategy::RealizedPnL, QColor(1,1,1));
    cards.append({"Positions",
                  info.value(MandatoryInfo::Strategy::PositionsCount, 0).toString(),
                  {}, MandatoryInfo::Strategy::PositionsCount});
    cards.append({"Open Orders",
                  info.value(MandatoryInfo::Strategy::OpenOrdersCount, 0).toString(),
                  {}, MandatoryInfo::Strategy::OpenOrdersCount});
    cards.append({"Drawdown",
                  info.value(MandatoryInfo::Strategy::DrawdownPct, "0.00").toString() + "%",
                  QColor(239, 68, 68), MandatoryInfo::Strategy::DrawdownPct});

    setMetrics(cards);

    refreshOverview();
    refreshInfo();
}

void StrategyWorkspace::refreshOverview()
{
    if (!m_boundModel) return;

    auto* baseModel = dynamic_cast<CBaseModel*>(m_boundModel);
    QVariantMap info = m_boundModel->genericInfo();

    if (baseModel) {
        auto ds = baseModel->resolveDisplayState();
        auto dispInfo = ModelStateUtils::stateDisplay(ds);
        m_ovStateSummary->setText(
            QStringLiteral("%1 %2").arg(dispInfo.indicator, dispInfo.label));
        m_ovStateSummary->setStyleSheet(
            QStringLiteral("color: %1; font-size: 14px;").arg(dispInfo.color.name()));
    }

    int posCount = info.value(MandatoryInfo::Strategy::PositionsCount).toInt();
    m_ovCurrentPos->setText(QStringLiteral("Positions: %1").arg(posCount));

    QString lastSignal = info.value(MandatoryInfo::Strategy::LastSignalTime).toString();
    m_ovLatestSignal->setText(
        QStringLiteral("Last signal: %1").arg(lastSignal.isEmpty() ? "--" : lastSignal));

    QString status = info.value(MandatoryInfo::Strategy::Status).toString();
    if (status == QLatin1String("Warning") || status == QLatin1String("Error"))
        m_ovWarnings->setText(QStringLiteral("Status: %1").arg(status));
    else
        m_ovWarnings->clear();

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (adapter) {
        auto p = Pipeline::StrategyRuntimePolicy::fromJson(
            adapter->pipelineConfig().value("runtimePolicy").toObject());
        m_ovEvalMode->setText(QStringLiteral("Evaluation: %1").arg(
            Pipeline::StrategyRuntimePolicy::evalModeLabel(p.evaluationMode)
            + (p.evaluationMode != Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose
                ? QStringLiteral(" (%1)").arg(p.evaluationIntervalN) : QString())));
        m_ovRebalMode->setText(QStringLiteral("Rebalance: %1").arg(
            Pipeline::StrategyRuntimePolicy::rebalModeLabel(p.rebalanceMode)
            + (p.rebalanceMode != Pipeline::StrategyRuntimePolicy::RebalanceMode::Immediate
                ? QStringLiteral(" (%1)").arg(p.rebalanceIntervalN) : QString())));
    }
}

void StrategyWorkspace::refreshProperties()
{
    if (!m_boundModel) return;

    while (m_propertiesForm->rowCount() > 0)
        m_propertiesForm->removeRow(0);

    const QVariantMap& params = m_boundModel->getParameters();
    for (auto it = params.cbegin(); it != params.cend(); ++it) {
        auto* edit = new QLineEdit(it.value().toString(), m_propertiesWidget);
        QString key = it.key();
        connect(edit, &QLineEdit::editingFinished, this, [this, edit, key]() {
            if (!m_boundModel) return;
            QVariantMap params = m_boundModel->getParameters();
            params[key] = edit->text();
            m_boundModel->setParameters(params);
        });
        m_propertiesForm->addRow(it.key(), edit);
    }
}

void StrategyWorkspace::refreshInfo()
{
    if (!m_boundModel) return;

    while (m_infoForm->rowCount() > 0)
        m_infoForm->removeRow(0);

    QVariantMap info = m_boundModel->genericInfo();
    for (auto it = info.cbegin(); it != info.cend(); ++it) {
        auto* label = new QLabel(it.value().toString(), m_infoWidget);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_infoForm->addRow(it.key(), label);
    }
}

void StrategyWorkspace::refreshAssets()
{
    if (!m_boundModel || !m_assetsEdit) return;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) {
        m_assetsEdit->clear();
        return;
    }

    const QJsonObject& cfg = adapter->pipelineConfig();
    QStringList allSymbols;

    auto extractSymbols = [&](const QJsonObject& entry) {
        QJsonObject blockCfg = entry.value("config").toObject();
        for (const auto& s : blockCfg.value("symbols").toArray())
            if (!allSymbols.contains(s.toString()))
                allSymbols.append(s.toString());
    };

    QJsonValue selVal = cfg.value("selection");
    if (selVal.isArray()) {
        for (const auto& entry : selVal.toArray())
            extractSymbols(entry.toObject());
    } else if (selVal.isObject()) {
        extractSymbols(selVal.toObject());
    }

    m_assetsEdit->setText(allSymbols.join(", "));

    auto resolved = Pipeline::UniverseResolver::resolve(cfg);
    m_universeInfoLabel->setText(resolved.reason);
}

void StrategyWorkspace::refreshPolicy()
{
    if (!m_boundModel || !m_policyEditor) return;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) return;

    m_policyEditor->loadFromJson(adapter->pipelineConfig());
}
