#include "StrategyWorkspace.h"
#include "StrategyWorkspacePresenter.h"
#include "LayoutConstants.h"
#include "MetricsStrip.h"
#include "WorkspaceHeader.h"
#include "BlockInspectorPanel.h"
#include "RuntimePolicyEditor.h"
#include "TradingReadiness.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include "cpipelinestrategyadapter.h"
#include "cbasemodel.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include "Pipeline/UniverseResolver.h"
#include "Pipeline/StrategyRuntimePolicy.h"
#include "ThemePalette.h"
#include "Backtest/AssetUniverseInput.h"
#include "Backtest/InstrumentClassification.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHash>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

using namespace Backtest;

QStringList pipelineSymbolsFromConfig(const QJsonObject& cfg)
{
    QStringList allSymbols;
    auto extract = [&](const QJsonObject& entry) {
        QJsonObject blockCfg = entry.value(QStringLiteral("config")).toObject();
        for (const auto& s : blockCfg.value(QStringLiteral("symbols")).toArray()) {
            const QString sym = s.toString().trimmed().toUpper();
            if (!sym.isEmpty() && !allSymbols.contains(sym))
                allSymbols.append(sym);
        }
    };
    QJsonValue selVal = cfg.value(QStringLiteral("selection"));
    if (selVal.isArray()) {
        for (const auto& entry : selVal.toArray())
            extract(entry.toObject());
    } else if (selVal.isObject()) {
        extract(selVal.toObject());
    }
    return allSymbols;
}

QString sourceLabel(const EffectiveInstrumentProfile& p)
{
    switch (p.provenance) {
    case InstrumentClassificationProvenance::ClassificationOverride:
        return QStringLiteral("User");
    case InstrumentClassificationProvenance::ProviderMetadata:
        return p.providerId.isEmpty() ? QStringLiteral("Provider") : p.providerId;
    case InstrumentClassificationProvenance::InferredHeuristic:
        return QStringLiteral("Inferred");
    case InstrumentClassificationProvenance::Indeterminate:
    default:
        return QStringLiteral("—");
    }
}

QString notesForProfile(const EffectiveInstrumentProfile& p)
{
    if (p.effectiveAssetKind != AssetKind::Unknown)
        return QString();
    if (p.classificationOverrideAssetKind.has_value())
        return QString();
    return QStringLiteral("Unresolved");
}

QStringList classificationOverrideComboItems()
{
    QStringList items;
    items << QStringLiteral("Auto");
    const AssetKind kinds[] = {AssetKind::Equity,  AssetKind::Etf,     AssetKind::Forex,
                               AssetKind::Crypto, AssetKind::Future,  AssetKind::Option,
                               AssetKind::Index,  AssetKind::Fund,    AssetKind::Bond};
    for (AssetKind k : kinds)
        items << assetKindToString(k);
    return items;
}

int comboIndexForOverride(const QString& classificationOverride)
{
    const QStringList items = classificationOverrideComboItems();
    const QString co      = classificationOverride.trimmed();
    if (co.isEmpty())
        return 0;
    for (int i = 1; i < items.size(); ++i) {
        if (items.at(i).compare(co, Qt::CaseInsensitive) == 0)
            return i;
    }
    return 0;
}

QString overrideKindForComboIndex(int idx)
{
    if (idx <= 0)
        return QString();
    const QStringList items = classificationOverrideComboItems();
    if (idx >= 0 && idx < items.size())
        return items.at(idx);
    return QString();
}

} // namespace

using namespace Backtest;

StrategyWorkspace::StrategyWorkspace(QWidget* parent)
    : WorkspaceBase(parent)
{
    m_presenter = new StrategyWorkspacePresenter(this);

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
    m_ovStateSummary->setObjectName(QStringLiteral("strategyOverviewState"));
    m_ovCurrentPos   = new QLabel("Positions: --", m_overviewWidget);
    m_ovLatestSignal = new QLabel("Last signal: --", m_overviewWidget);
    m_ovWarnings     = new QLabel("", m_overviewWidget);
    m_ovWarnings->setObjectName(QStringLiteral("strategyOverviewWarning"));

    m_ovEvalMode  = new QLabel("Evaluation: --", m_overviewWidget);
    m_ovEvalMode->setObjectName(QStringLiteral("strategyOverviewMuted"));
    m_ovRebalMode = new QLabel("Rebalance: --", m_overviewWidget);
    m_ovRebalMode->setObjectName(QStringLiteral("strategyOverviewMuted"));

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

    layout->addSpacing(Layout::SectionSpacing);
    layout->addWidget(new QLabel("<b>Trading Readiness</b>", m_overviewWidget));

    auto* modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel(QStringLiteral("Execution mode:"), m_overviewWidget));
    m_executionModeCombo = new QComboBox(m_overviewWidget);
    m_executionModeCombo->addItem(QStringLiteral("Dry Run"), QStringLiteral("dry_run"));
    m_executionModeCombo->addItem(QStringLiteral("Live Orders"), QStringLiteral("live"));
    modeRow->addWidget(m_executionModeCombo, 1);
    layout->addLayout(modeRow);

    m_readyBroker = new QLabel(m_overviewWidget);
    m_readyData = new QLabel(m_overviewWidget);
    m_readyExecution = new QLabel(m_overviewWidget);
    m_readyStrategy = new QLabel(m_overviewWidget);
    m_readyUniverse = new QLabel(m_overviewWidget);
    m_readySummary = new QLabel(m_overviewWidget);
    m_readySummary->setWordWrap(true);
    m_readySummary->setObjectName(QStringLiteral("strategyOverviewWarning"));
    for (QLabel* lbl : {m_readyBroker, m_readyData, m_readyExecution, m_readyStrategy, m_readyUniverse})
        lbl->setObjectName(QStringLiteral("strategyOverviewMuted"));
    layout->addWidget(m_readyBroker);
    layout->addWidget(m_readyData);
    layout->addWidget(m_readyExecution);
    layout->addWidget(m_readyStrategy);
    layout->addWidget(m_readyUniverse);
    layout->addWidget(m_readySummary);

    connect(m_executionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StrategyWorkspace::onExecutionModeChanged);

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

    auto* hint = new QLabel(AssetUniverseInput::assetsTabDescription(), m_assetsWidget);
    hint->setObjectName(QStringLiteral("workspaceFormHint"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    layout->addWidget(new QLabel("<b>Asset Universe</b>", m_assetsWidget));

    {
        auto* h = new QHBoxLayout();
        h->addWidget(new QLabel(QStringLiteral("Provider metadata:"), m_assetsWidget));
        m_assetsProviderCombo = new QComboBox(m_assetsWidget);
        m_assetsProviderCombo->addItem(QStringLiteral("Yahoo (backtest charts)"), QStringLiteral("yahoo"));
        m_assetsProviderCombo->addItem(QStringLiteral("Interactive Brokers (live)"), QStringLiteral("ib"));
        m_assetsProviderCombo->setToolTip(
            QStringLiteral("Which InstrumentMetadata provider row to use for Effective type / Source when the database has rows."));
        h->addWidget(m_assetsProviderCombo, 1);
        layout->addLayout(h);
        connect(m_assetsProviderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            if (m_assetsTableUpdating)
                return;
            auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
            if (!adapter)
                return;
            const QString    id = m_assetsProviderCombo->itemData(idx).toString();
            QVariantMap      p  = adapter->getParameters();
            p[QStringLiteral("instrumentMetadataProviderId")] = id;
            adapter->setParameters(p);
            rebuildAssetsTable();
        });
    }

    m_assetsEdit = new QLineEdit(m_assetsWidget);
    m_assetsEdit->setPlaceholderText(AssetUniverseInput::lineEditPlaceholder());
    m_assetsEdit->setToolTip(AssetUniverseInput::lineEditToolTip());
    connect(m_assetsEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_boundModel) return;
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
        if (!adapter) return;

        const auto parsed = AssetUniverseInput::parseLine(m_assetsEdit->text());
        const QStringList& symbolOrder = parsed.symbolOrder;
        const QHash<QString, QString>& classificationBySymbol = parsed.classificationOverrideBySymbol;

        QJsonArray symArr;
        for (const QString& s : symbolOrder)
            symArr.append(s);

        QJsonObject cfg     = adapter->pipelineConfig();
        QJsonValue  selVal = cfg.value(QStringLiteral("selection"));

        if (selVal.isArray()) {
            QJsonArray arr = selVal.toArray();
            if (!arr.isEmpty()) {
                QJsonObject entry    = arr[0].toObject();
                QJsonObject blockCfg = entry.value(QStringLiteral("config")).toObject();
                blockCfg[QStringLiteral("symbols")] = symArr;
                entry[QStringLiteral("config")]     = blockCfg;
                arr[0]                              = entry;
                cfg[QStringLiteral("selection")]    = arr;
            }
        } else if (selVal.isObject()) {
            QJsonObject entry    = selVal.toObject();
            QJsonObject blockCfg = entry.value(QStringLiteral("config")).toObject();
            blockCfg[QStringLiteral("symbols")] = symArr;
            entry[QStringLiteral("config")]     = blockCfg;
            cfg[QStringLiteral("selection")]    = entry;
        }

        QVariantMap assetList = adapter->assetList();
        for (auto it = assetList.begin(); it != assetList.end();) {
            if (!symbolOrder.contains(it.key()))
                it = assetList.erase(it);
            else
                ++it;
        }

        auto* base = static_cast<CBaseModel*>(adapter);
        for (const QString& sym : symbolOrder) {
            QVariantMap entry = assetList.value(sym).toMap();
            if (classificationBySymbol.contains(sym))
                entry[QString::fromUtf8(AssetFields::Position::ClassificationOverride)] =
                    classificationBySymbol.value(sym);
            else
                entry.remove(QString::fromUtf8(AssetFields::Position::ClassificationOverride));
            assetList.insert(sym, base->createAssetEntry(entry));
        }
        adapter->setAssetList(assetList);
        adapter->setPipelineConfig(cfg);
        rebuildAssetsTable();
    });
    layout->addWidget(m_assetsEdit);

    m_assetsTableHint = new QLabel(
        "Table: effective type and source use the instrument registry when the app database is connected. "
        "Choose Yahoo vs IB above to match backtest vs live metadata. Override is a hard classification override for this strategy.",
        m_assetsWidget);
    m_assetsTableHint->setObjectName(QStringLiteral("workspaceFormHint"));
    m_assetsTableHint->setWordWrap(true);
    layout->addWidget(m_assetsTableHint);

    m_assetsTable = new QTableWidget(0, 6, m_assetsWidget);
    m_assetsTable->setHorizontalHeaderLabels({QStringLiteral("Symbol"),
                                              QStringLiteral("Effective type"),
                                              QStringLiteral("Source"),
                                              QStringLiteral("Override"),
                                              QStringLiteral("Session policy"),
                                              QStringLiteral("Notes")});
    m_assetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_assetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_assetsTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                     | QAbstractItemView::SelectedClicked);
    layout->addWidget(m_assetsTable);

    connect(m_assetsTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (m_assetsTableUpdating || !item || item->column() != 4)
            return;
        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
        if (!adapter)
            return;
        const QString sym = item->data(Qt::UserRole).toString();
        if (sym.isEmpty())
            return;
        auto* base     = static_cast<CBaseModel*>(adapter);
        QVariantMap assetList = adapter->assetList();
        QVariantMap entry     = assetList.value(sym).toMap();
        const QString pol     = item->text().trimmed();
        if (pol.isEmpty())
            entry.remove(AssetFields::Position::SessionPolicy);
        else
            entry[AssetFields::Position::SessionPolicy] = pol;
        assetList.insert(sym, base->createAssetEntry(entry));
        adapter->setAssetList(assetList);
        {
            QSignalBlocker b(m_assetsEdit);
            m_assetsEdit->setText(
                AssetUniverseInput::formatLine(pipelineSymbolsFromConfig(adapter->pipelineConfig()), assetList));
        }
    });

    m_universeInfoLabel = new QLabel(m_assetsWidget);
    m_universeInfoLabel->setObjectName(QStringLiteral("workspaceFormHint"));
    m_universeInfoLabel->setWordWrap(true);
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
    m_presenter->bind(m_boundModel, m_parentModel);

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
    syncAssetsProviderComboFromModel();
    refreshAssets();
    refreshPolicy();
    restoreStrategyProperties();
    m_tabWidget->setCurrentIndex(0);
}

void StrategyWorkspace::onContextCleared()
{
    m_presenter->unbind();

    m_header->clear();
    m_metricsStrip->clear();
    m_ovStateSummary->setText("--");
    m_ovCurrentPos->setText("Positions: --");
    m_ovLatestSignal->setText("Last signal: --");
    m_ovEvalMode->setText("Evaluation: --");
    m_ovRebalMode->setText("Rebalance: --");
    m_ovWarnings->clear();
    if (m_readyBroker) m_readyBroker->setText("Broker: --");
    if (m_readyData) m_readyData->setText("Data: --");
    if (m_readyExecution) m_readyExecution->setText("Execution: --");
    if (m_readyStrategy) m_readyStrategy->setText("Strategy: --");
    if (m_readyUniverse) m_readyUniverse->setText("Universe: --");
    if (m_readySummary) m_readySummary->clear();
    m_logsText->clear();

    while (m_propertiesForm->rowCount() > 0)
        m_propertiesForm->removeRow(0);

    while (m_infoForm->rowCount() > 0)
        m_infoForm->removeRow(0);

    if (m_assetsEdit)
        m_assetsEdit->clear();
    if (m_assetsTable)
        m_assetsTable->setRowCount(0);
    if (m_universeInfoLabel)
        m_universeInfoLabel->clear();
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
        m_ovStateSummary->setStyleSheet(QStringLiteral(
            "QLabel#strategyOverviewState { color: %1; font-size: %2px; }")
            .arg(dispInfo.color.name())
            .arg(UiTheme::kFontSizeStrategyOverviewState));
    } else {
        m_ovStateSummary->setStyleSheet(QString());
        m_ovStateSummary->setText(QStringLiteral("--"));
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
    syncExecutionModeControls();
    refreshTradingReadiness();
}

void StrategyWorkspace::syncExecutionModeControls()
{
    if (!m_executionModeCombo)
        return;
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter)
        return;
    const QString mode = adapter->executionMode() == CPipelineStrategyAdapter::ExecutionMode::Live
        ? QStringLiteral("live")
        : QStringLiteral("dry_run");
    const int idx = m_executionModeCombo->findData(mode);
    if (idx >= 0 && idx != m_executionModeCombo->currentIndex()) {
        QSignalBlocker b(m_executionModeCombo);
        m_updatingExecutionMode = true;
        m_executionModeCombo->setCurrentIndex(idx);
        m_updatingExecutionMode = false;
    }
}

void StrategyWorkspace::refreshTradingReadiness()
{
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter)
        return;

    const bool brokerConnected = CPipelineStrategyAdapter::isGlobalBrokerConnected();
    const bool liveOrders = adapter->executionMode() == CPipelineStrategyAdapter::ExecutionMode::Live;
    const bool strategyRunning =
        adapter->genericInfo().value(MandatoryInfo::Strategy::Status).toString() == QLatin1String("Running");
    const bool portsReady =
        CPipelineStrategyAdapter::hasGlobalExecutionPort() && CPipelineStrategyAdapter::hasGlobalPositionRepo();

    const TradingUX::ReadinessState state = TradingUX::computeLiveReadiness(
        adapter->pipelineConfig(), liveOrders, strategyRunning, brokerConnected, false, portsReady);

    if (m_readyBroker) m_readyBroker->setText(TradingUX::brokerText(state));
    if (m_readyData) m_readyData->setText(TradingUX::dataText(state));
    if (m_readyExecution) m_readyExecution->setText(TradingUX::executionText(state));
    if (m_readyStrategy) m_readyStrategy->setText(TradingUX::strategyText(state));
    if (m_readyUniverse) m_readyUniverse->setText(TradingUX::universeText(state));
    if (m_readySummary) m_readySummary->setText(state.summaryText());

    if (m_executionModeCombo) {
        const QString tip = brokerConnected
            ? QStringLiteral("Dry Run simulates orders. Live Orders sends approved orders to IB/TWS.")
            : QStringLiteral("Connect IB/TWS before selecting Live Orders.");
        m_executionModeCombo->setToolTip(tip);
    }
}

void StrategyWorkspace::onExecutionModeChanged(int index)
{
    if (m_updatingExecutionMode)
        return;
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter || !m_executionModeCombo)
        return;

    const QString requested = m_executionModeCombo->itemData(index).toString();
    const bool wantsLive = requested == QLatin1String("live");
    if (wantsLive && !CPipelineStrategyAdapter::isGlobalBrokerConnected()) {
        QMessageBox::warning(this, QStringLiteral("Live orders"),
                             QStringLiteral("Connect IB/TWS before enabling live orders."));
        syncExecutionModeControls();
        refreshTradingReadiness();
        return;
    }

    if (wantsLive) {
        const auto choice = QMessageBox::question(
            this, QStringLiteral("Enable live orders"),
            QStringLiteral("Live Orders will send approved orders to IB/TWS. Continue?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            syncExecutionModeControls();
            refreshTradingReadiness();
            return;
        }
    }

    QVariantMap params = adapter->getParameters();
    params[QStringLiteral("execution_mode")] = wantsLive ? QStringLiteral("live")
                                                         : QStringLiteral("dry_run");
    adapter->setParameters(params);
    syncExecutionModeControls();
    refreshTradingReadiness();
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

void StrategyWorkspace::syncAssetsProviderComboFromModel()
{
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter || !m_assetsProviderCombo)
        return;
    const QString v =
        adapter->getParameters().value(QStringLiteral("instrumentMetadataProviderId")).toString();
    const bool ib = (v.compare(QLatin1String("ib"), Qt::CaseInsensitive) == 0);
    QSignalBlocker b(m_assetsProviderCombo);
    m_assetsProviderCombo->setCurrentIndex(ib ? 1 : 0);
}

void StrategyWorkspace::rebuildAssetsTable()
{
    if (!m_assetsTable || !m_boundModel)
        return;
    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) {
        m_assetsTable->setRowCount(0);
        return;
    }

    const QStringList symbols   = pipelineSymbolsFromConfig(adapter->pipelineConfig());
    const QVariantMap assetList = adapter->assetList();
    const QString dbConn = static_cast<CBaseModel*>(adapter)->databaseConnectionName();
    const QString providerId =
        m_assetsProviderCombo ? m_assetsProviderCombo->currentData().toString() : QStringLiteral("yahoo");

    m_assetsTableUpdating = true;
    m_assetsTable->setRowCount(0);

    int row = 0;
    for (const QString& sym : symbols) {
        m_assetsTable->insertRow(row);
        const QVariantMap entry = assetList.value(sym).toMap();

        auto* symItem = new QTableWidgetItem(sym);
        symItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        symItem->setData(Qt::UserRole, sym);
        m_assetsTable->setItem(row, 0, symItem);

        const EffectiveInstrumentProfile prof =
            InstrumentMetadataResolver::resolve(dbConn, sym, providerId, entry);

        auto* effItem = new QTableWidgetItem(assetKindToString(prof.effectiveAssetKind));
        effItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_assetsTable->setItem(row, 1, effItem);

        auto* srcItem = new QTableWidgetItem(sourceLabel(prof));
        srcItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_assetsTable->setItem(row, 2, srcItem);

        auto* combo = new QComboBox(m_assetsTable);
        combo->addItems(classificationOverrideComboItems());
        {
            QSignalBlocker cb(combo);
            combo->setCurrentIndex(
                comboIndexForOverride(entry.value(AssetFields::Position::ClassificationOverride).toString()));
        }
        const QString symCopy = sym;
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, symCopy](int idx) {
            if (m_assetsTableUpdating)
                return;
            auto* ad = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
            if (!ad)
                return;
            auto* base = static_cast<CBaseModel*>(ad);
            QVariantMap al  = ad->assetList();
            QVariantMap ent = al.value(symCopy).toMap();
            const QString k = overrideKindForComboIndex(idx);
            if (k.isEmpty())
                ent.remove(AssetFields::Position::ClassificationOverride);
            else
                ent[AssetFields::Position::ClassificationOverride] = k;
            al.insert(symCopy, base->createAssetEntry(ent));
            ad->setAssetList(al);
            {
                QSignalBlocker b(m_assetsEdit);
                m_assetsEdit->setText(
                    AssetUniverseInput::formatLine(pipelineSymbolsFromConfig(ad->pipelineConfig()), al));
            }
            rebuildAssetsTable();
        });
        m_assetsTable->setCellWidget(row, 3, combo);

        const QString sess = entry.value(AssetFields::Position::SessionPolicy).toString();
        auto* sessItem     = new QTableWidgetItem(sess);
        sessItem->setData(Qt::UserRole, sym);
        sessItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        m_assetsTable->setItem(row, 4, sessItem);

        auto* noteItem = new QTableWidgetItem(notesForProfile(prof));
        noteItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_assetsTable->setItem(row, 5, noteItem);

        ++row;
    }
    m_assetsTableUpdating = false;
}

void StrategyWorkspace::refreshAssets()
{
    if (!m_boundModel || !m_assetsEdit)
        return;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) {
        m_assetsEdit->clear();
        if (m_assetsTable)
            m_assetsTable->setRowCount(0);
        return;
    }

    const QJsonObject cfg        = adapter->pipelineConfig();
    const QStringList allSymbols = pipelineSymbolsFromConfig(cfg);
    const QVariantMap assetList  = adapter->assetList();

    {
        QSignalBlocker b(m_assetsEdit);
        m_assetsEdit->setText(AssetUniverseInput::formatLine(allSymbols, assetList));
    }

    auto resolved = Pipeline::UniverseResolver::resolve(cfg);
    m_universeInfoLabel->setText(resolved.reason);
    rebuildAssetsTable();
}

void StrategyWorkspace::refreshPolicy()
{
    if (!m_boundModel || !m_policyEditor) return;

    auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(m_boundModel);
    if (!adapter) return;

    m_policyEditor->loadFromJson(adapter->pipelineConfig());
}
