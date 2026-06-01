#include "StrategyDetailPanel.h"
#include "StrategyDetailPresenter.h"
#include "BlockInspectorPanel.h"
#include "RuntimePolicyEditor.h"

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTabWidget>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QMenu>

namespace StrategyMgmt {

static QString lifecycleDisplayLabel(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("testing"))
        return QStringLiteral("Testing");
    if (normalized == QStringLiteral("ready"))
        return QStringLiteral("Ready");
    if (normalized == QStringLiteral("retired"))
        return QStringLiteral("Retired");
    return QStringLiteral("Draft");
}

static QString versionAvailabilityLabel(const QJsonObject& version)
{
    return version.value(QStringLiteral("isPublished")).toBool()
        ? QStringLiteral("Published")
        : QStringLiteral("Unpublished");
}

StrategyDetailPanel::StrategyDetailPanel(QWidget* parent)
    : QWidget(parent)
{
    m_detailPresenter = new StrategyDetailPresenter(this);
    buildUi();
}

void StrategyDetailPanel::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // --- Strategy header / metadata ---
    auto* headerGroup = new QGroupBox(QStringLiteral("Strategy"));
    auto* headerLayout = new QVBoxLayout(headerGroup);
    headerLayout->setContentsMargins(8, 8, 8, 8);
    headerLayout->setSpacing(6);

    m_breadcrumbLabel = new QLabel;
    m_breadcrumbLabel->setObjectName(QStringLiteral("StrategyBreadcrumbLabel"));
    headerLayout->addWidget(m_breadcrumbLabel);

    auto* titleRow = new QHBoxLayout;
    m_titleLabel = new QLabel;
    m_titleLabel->setObjectName(QStringLiteral("StrategyTitleLabel"));

    m_statusCombo = new QComboBox;
    m_statusCombo->addItem(QStringLiteral("Draft"), QStringLiteral("draft"));
    m_statusCombo->addItem(QStringLiteral("Testing"), QStringLiteral("testing"));
    m_statusCombo->addItem(QStringLiteral("Ready"), QStringLiteral("ready"));
    m_statusCombo->addItem(QStringLiteral("Retired"), QStringLiteral("retired"));
    m_statusCombo->setToolTip(
        QStringLiteral("Manual lifecycle for the strategy: draft, testing, ready, or retired."));

    titleRow->addWidget(m_titleLabel, 1);
    titleRow->addWidget(new QLabel(QStringLiteral("Lifecycle:")));
    titleRow->addWidget(m_statusCombo);
    headerLayout->addLayout(titleRow);

    m_headerHintLabel = new QLabel;
    m_headerHintLabel->setObjectName(QStringLiteral("StrategyHeaderHintLabel"));
    headerLayout->addWidget(m_headerHintLabel);

    auto* metaForm = new QFormLayout;
    metaForm->setContentsMargins(0, 0, 0, 0);

    m_nameEdit = new QLineEdit;
    metaForm->addRow(QStringLiteral("Name:"), m_nameEdit);

    m_descEdit = new QTextEdit;
    m_descEdit->setMaximumHeight(54);
    metaForm->addRow(QStringLiteral("Description:"), m_descEdit);

    auto* tagsRow = new QHBoxLayout;
    m_tagsEdit = new QLineEdit;
    m_tagsEdit->setPlaceholderText(QStringLiteral("comma-separated tags"));
    m_saveMetaBtn = new QPushButton(QStringLiteral("Save metadata"));
    tagsRow->addWidget(m_tagsEdit, 1);
    tagsRow->addWidget(m_saveMetaBtn);
    metaForm->addRow(QStringLiteral("Tags:"), tagsRow);
    headerLayout->addLayout(metaForm);

    mainLayout->addWidget(headerGroup);

    // --- Version table ---
    auto* verGroup = new QGroupBox(QStringLiteral("Versions"));
    auto* verLayout = new QVBoxLayout(verGroup);

    m_versionTable = new QTableWidget;
    m_versionTable->setColumnCount(4);
    m_versionTable->setHorizontalHeaderLabels({
        QStringLiteral("Version"),
        QStringLiteral("Published"),
        QStringLiteral("Notes"),
        QStringLiteral("Created")
    });
    m_versionTable->horizontalHeader()->setStretchLastSection(true);
    m_versionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_versionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_versionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_versionTable->verticalHeader()->hide();
    m_versionTable->setMaximumHeight(120);
    verLayout->addWidget(m_versionTable);

    m_versionEditStateLabel = new QLabel;
    m_versionEditStateLabel->setObjectName(QStringLiteral("VersionEditStateLabel"));
    verLayout->addWidget(m_versionEditStateLabel);

    mainLayout->addWidget(verGroup);

    // --- Tabs: Raw JSON, Runtime Policy, Block Details ---
    m_configTabs = new QTabWidget;

    auto* jsonTab = new QWidget;
    auto* jsonLayout = new QVBoxLayout(jsonTab);
    jsonLayout->setContentsMargins(4, 4, 4, 4);

    m_diffToggle = new QCheckBox(QStringLiteral("Diff vs Previous"));
    jsonLayout->addWidget(m_diffToggle);

    m_configViewer = new QPlainTextEdit;
    m_configViewer->setReadOnly(true);
    jsonLayout->addWidget(m_configViewer);

    m_configTabs->addTab(jsonTab, QStringLiteral("Raw JSON"));

    // Runtime Policy tab (wrapped in scroll area to avoid
    // inflating the QTabWidget's minimum size)
    m_policyEditor = new RuntimePolicyEditor;
    auto* policyScroll = new QScrollArea;
    policyScroll->setWidgetResizable(true);
    policyScroll->setFrameShape(QFrame::NoFrame);
    policyScroll->setWidget(m_policyEditor);
    m_configTabs->addTab(policyScroll, QStringLiteral("Runtime Policy"));

    connect(m_policyEditor, &RuntimePolicyEditor::policyChanged,
            this, [this]() {
        if (currentVersionPublished()) {
            m_policyEditor->loadFromJson(m_workingConfig);
            m_policyEditor->setReadOnly(true);
            updateVersionEditLock();
            return;
        }
        m_workingConfig = m_policyEditor->applyToJson(m_workingConfig);
        markDirty();
    });

    m_inspector = new BlockInspectorPanel;
    auto* inspectorScroll = new QScrollArea;
    inspectorScroll->setWidgetResizable(true);
    inspectorScroll->setFrameShape(QFrame::NoFrame);
    inspectorScroll->setWidget(m_inspector);
    m_inspectorTabIdx = m_configTabs->addTab(inspectorScroll, QStringLiteral("Block Details"));

    // Inspector param edits update working config
    connect(m_inspector, &BlockInspectorPanel::configChanged,
            this, [this](const QJsonObject& newCfg) {
        if (currentVersionPublished()) {
            updateVersionEditLock();
            return;
        }
        m_workingConfig = newCfg;
        markDirty();
    });

    mainLayout->addWidget(m_configTabs, 1);

    // --- Action buttons ---
    auto* actionBar = new QHBoxLayout;

    m_newVersionBtn = new QPushButton(QStringLiteral("Save as New Version"));
    m_publishBtn    = new QPushButton(QStringLiteral("Publish version"));
    m_unpublishBtn  = new QPushButton(QStringLiteral("Unpublish version"));
    m_moreActionsBtn = new QPushButton(QStringLiteral("More"));
    m_useInLiveBtn  = new QPushButton(QStringLiteral("Use in Live"));
    m_openBtBtn     = new QPushButton(QStringLiteral("Open in Backtest"));
    m_publishBtn->setToolTip(
        QStringLiteral("Published versions are locked and can be used live. Unpublished versions remain editable and can still be backtested."));
    m_unpublishBtn->setToolTip(
        QStringLiteral("Unpublish keeps the version for editing/backtesting, but removes it from live deployment choices."));

    auto* moreMenu = new QMenu(m_moreActionsBtn);
    m_deleteVersionAction =
        moreMenu->addAction(QStringLiteral("Delete selected version..."),
                            this, &StrategyDetailPanel::onDeleteVersion);
    m_archiveAction =
        moreMenu->addAction(QStringLiteral("Retire strategy..."),
                            this, &StrategyDetailPanel::onArchive);
    m_moreActionsBtn->setMenu(moreMenu);

    actionBar->addWidget(m_newVersionBtn);
    actionBar->addWidget(m_publishBtn);
    actionBar->addWidget(m_unpublishBtn);
    actionBar->addWidget(m_moreActionsBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_useInLiveBtn);
    actionBar->addWidget(m_openBtBtn);

    mainLayout->addLayout(actionBar);

    // --- Connections ---
    connect(m_versionTable, &QTableWidget::currentCellChanged,
            this, &StrategyDetailPanel::onVersionCurrentCellChanged);
    connect(m_nameEdit, &QLineEdit::editingFinished,
            this, &StrategyDetailPanel::onSaveMetadata);
    connect(m_statusCombo, &QComboBox::currentTextChanged,
            this, [this](const QString&) { onSaveMetadata(); });
    connect(m_saveMetaBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onSaveMetadata);
    connect(m_newVersionBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onNewVersion);
    connect(m_publishBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onPublish);
    connect(m_unpublishBtn, &QPushButton::clicked,
            this, [this]() {
        const QString vId = selectedVersionId();
        if (!vId.isEmpty())
            emit unpublishRequested(m_currentStrategyId, vId);
    });
    connect(m_useInLiveBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onUseInLive);
    connect(m_openBtBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onOpenInBacktest);
    connect(m_diffToggle, &QCheckBox::toggled,
            this, [this](bool) {
        int row = m_versionTable->currentRow();
        if (row >= 0) loadVersionAtRow(row);
    });

    clear();
}

void StrategyDetailPanel::showStrategy(const QJsonObject& catalogEntry,
                                        const QJsonArray& versions)
{
    m_currentStrategyId = catalogEntry.value("strategyId").toString();
    m_currentVersions   = versions;

    setEnabled(true);

    const QString strategyName = catalogEntry.value("name").toString();
    m_titleLabel->setText(strategyName);
    m_breadcrumbLabel->setText(strategyName);
    m_nameEdit->setText(strategyName);
    m_descEdit->setPlainText(catalogEntry.value("description").toString());
    m_tagsEdit->setText(catalogEntry.value("tags").toString());
    updateStrategyHeaderHint(catalogEntry);

    QString state = catalogEntry.value("lifecycleState").toString();
    int idx = m_statusCombo->findData(state);
    {
        QSignalBlocker blocker(m_statusCombo);
        m_statusCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }

    m_versionTable->setRowCount(versions.size());
    for (int i = 0; i < versions.size(); ++i) {
        QJsonObject v = versions[i].toObject();
        m_versionTable->setItem(i, 0, new QTableWidgetItem(
            QStringLiteral("v%1").arg(v.value("versionNumber").toInt())));
        auto* publishedItem = new QTableWidgetItem(versionAvailabilityLabel(v));
        publishedItem->setForeground(v.value("isPublished").toBool()
                                         ? QColor(QStringLiteral("#7bd88f"))
                                         : QColor(QStringLiteral("#a0a0a0")));
        m_versionTable->setItem(i, 1, publishedItem);
        m_versionTable->setItem(i, 2, new QTableWidgetItem(
            v.value("notes").toString()));
        m_versionTable->setItem(i, 3, new QTableWidgetItem(
            v.value("createdAt").toString()));
    }

    m_configViewer->clear();
    hideBlockDetails();

    if (versions.size() > 0) {
        selectVersionRow(versions.size() - 1);
    } else {
        {
            QSignalBlocker blocker(m_versionTable);
            m_versionTable->clearSelection();
            m_versionTable->setCurrentItem(nullptr);
        }
        m_workingConfig = QJsonObject();
        m_configDirty = false;
        m_policyEditor->loadFromJson(m_workingConfig);
        updateNewVersionButtonText();
        updateVersionActionState();
    }
}

void StrategyDetailPanel::clear()
{
    m_currentStrategyId.clear();
    m_currentVersions = QJsonArray();
    m_breadcrumbLabel->clear();
    m_titleLabel->clear();
    m_headerHintLabel->clear();
    m_nameEdit->clear();
    m_descEdit->clear();
    m_tagsEdit->clear();
    {
        QSignalBlocker blocker(m_statusCombo);
        m_statusCombo->setCurrentIndex(0);
    }
    m_versionTable->setRowCount(0);
    m_configViewer->clear();
    m_versionEditStateLabel->clear();
    hideBlockDetails();
    m_workingConfig = QJsonObject();
    m_configDirty = false;
    updateNewVersionButtonText();
    updateVersionActionState();
    setEnabled(false);
}

void StrategyDetailPanel::setSelectedBlockContext(const QString& category,
                                                  const QString& blockName)
{
    if (!m_breadcrumbLabel || m_titleLabel->text().isEmpty())
        return;
    if (category.isEmpty() || blockName.isEmpty()) {
        m_breadcrumbLabel->setText(m_titleLabel->text());
        return;
    }
    m_breadcrumbLabel->setText(
        QStringLiteral("%1 / %2 / %3")
            .arg(m_titleLabel->text(), category, blockName));
}

void StrategyDetailPanel::showBlockDetails(const QString& category,
                                            const QString& jsonKey,
                                            bool isArray, int arrayIndex)
{
    if (m_workingConfig.isEmpty()) return;

    m_configTabs->setCurrentIndex(m_inspectorTabIdx);
    m_inspector->setReadOnly(currentVersionPublished());
    m_inspector->showBlock(m_workingConfig, category, jsonKey, isArray, arrayIndex);
}

void StrategyDetailPanel::hideBlockDetails()
{
    m_inspector->clear();
}

void StrategyDetailPanel::onVersionCurrentCellChanged(int currentRow, int /*currentColumn*/,
                                                      int previousRow, int /*previousColumn*/)
{
    if (m_suppressVersionNav)
        return;
    if (currentRow < 0 || currentRow == previousRow)
        return;
    if (!m_configDirty) {
        loadVersionAtRow(currentRow);
        return;
    }
    if (previousRow < 0) {
        loadVersionAtRow(currentRow);
        return;
    }
    emit versionRowChangeRequested(currentRow, previousRow);
}

void StrategyDetailPanel::selectVersionRow(int row)
{
    if (row < 0 || row >= m_currentVersions.size())
        return;

    const QSignalBlocker blocker(m_versionTable);
    m_versionTable->selectRow(row);
    loadVersionAtRow(row);
}

void StrategyDetailPanel::loadVersionAtRow(int row)
{
    if (row < 0 || row >= m_currentVersions.size()) {
        m_configViewer->clear();
        updateVersionActionState();
        return;
    }

    QJsonObject v = m_currentVersions[row].toObject();
    QString configJson = v.value("configJson").toString();
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());

    m_workingConfig = doc.object();
    m_configDirty = false;
    updateNewVersionButtonText();
    hideBlockDetails();

    m_policyEditor->loadFromJson(m_workingConfig);
    updateVersionEditLock();

    QString currentText = doc.toJson(QJsonDocument::Indented);

    if (m_diffToggle->isChecked() && row > 0) {
        QJsonObject prev = m_currentVersions[row - 1].toObject();
        QString prevJson = prev.value("configJson").toString();
        QJsonDocument prevDoc = QJsonDocument::fromJson(prevJson.toUtf8());
        QString prevText = prevDoc.toJson(QJsonDocument::Indented);

        QStringList curLines  = currentText.split('\n');
        QStringList prevLines = prevText.split('\n');

        QString diff;
        int maxLines = qMax(curLines.size(), prevLines.size());
        for (int i = 0; i < maxLines; ++i) {
            QString cl = (i < curLines.size())  ? curLines[i]  : QString();
            QString pl = (i < prevLines.size()) ? prevLines[i] : QString();
            if (cl == pl) {
                diff += QStringLiteral("  ") + cl + '\n';
            } else {
                if (!pl.isEmpty())
                    diff += QStringLiteral("- ") + pl + '\n';
                if (!cl.isEmpty())
                    diff += QStringLiteral("+ ") + cl + '\n';
            }
        }
        m_configViewer->setPlainText(diff);
    } else {
        m_configViewer->setPlainText(currentText);
    }

    updateVersionActionState();
}

void StrategyDetailPanel::resetWorkingToSelectedVersion()
{
    const int row = m_versionTable->currentRow();
    if (row < 0 || row >= m_currentVersions.size())
        return;
    loadVersionAtRow(row);
}

void StrategyDetailPanel::setWorkingPipelineConfig(const QJsonObject& pipelineConfig)
{
    if (currentVersionPublished()) {
        updateVersionEditLock();
        return;
    }

    m_workingConfig = pipelineConfig;
    m_policyEditor->loadFromJson(m_workingConfig);
    hideBlockDetails();
    markDirty();
}

void StrategyDetailPanel::markDirty()
{
    m_configDirty = true;
    updateNewVersionButtonText();

    QJsonDocument doc(m_workingConfig);
    m_configViewer->setPlainText(doc.toJson(QJsonDocument::Indented));
    updateVersionActionState();
}

// --- Metadata & actions ---

void StrategyDetailPanel::onSaveMetadata()
{
    if (m_currentStrategyId.isEmpty()) return;
    emit metadataChanged(m_currentStrategyId,
                         m_nameEdit->text().trimmed(),
                         m_descEdit->toPlainText(),
                         m_tagsEdit->text(),
                         m_statusCombo->currentData().toString());
}

void StrategyDetailPanel::onNewVersion()
{
    if (m_currentStrategyId.isEmpty()) return;
    emit newVersionRequested(m_currentStrategyId, m_workingConfig);
}

void StrategyDetailPanel::onPublish()
{
    QString vId = selectedVersionId();
    if (!vId.isEmpty())
        emit publishRequested(m_currentStrategyId, vId);
}

void StrategyDetailPanel::onDeleteVersion()
{
    const QString vId = selectedVersionId();
    if (!vId.isEmpty())
        emit deleteVersionRequested(m_currentStrategyId, vId);
}

void StrategyDetailPanel::onArchive()
{
    if (!m_currentStrategyId.isEmpty())
        emit archiveRequested(m_currentStrategyId);
}

void StrategyDetailPanel::onUseInLive()
{
    QString vId = selectedVersionId();
    if (!vId.isEmpty())
        emit useInLiveRequested(m_currentStrategyId, vId);
}

void StrategyDetailPanel::onOpenInBacktest()
{
    QString vId = selectedVersionId();
    if (!vId.isEmpty())
        emit openInBacktestRequested(m_currentStrategyId, vId);
}

QString StrategyDetailPanel::selectedVersionId() const
{
    if (!m_versionTable || !m_versionTable->selectionModel())
        return {};
    const QModelIndexList rows = m_versionTable->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return {};
    const int row = rows.first().row();
    if (row < 0 || row >= m_currentVersions.size())
        return {};
    return m_currentVersions[row].toObject().value("versionId").toString();
}

bool StrategyDetailPanel::selectVersionById(const QString& versionId)
{
    if (versionId.isEmpty())
        return false;
    for (int row = 0; row < m_currentVersions.size(); ++row) {
        if (m_currentVersions[row].toObject().value(QStringLiteral("versionId")).toString()
            == versionId) {
            selectVersionRow(row);
            return true;
        }
    }
    return false;
}

bool StrategyDetailPanel::currentVersionPublished() const
{
    return selectedVersionPublished();
}

void StrategyDetailPanel::updateVersionActionState()
{
    const bool hasSelectedVersion = !selectedVersionId().isEmpty();
    const bool selectedPublished = selectedVersionPublished();
    const bool hasDiffBase = hasSelectedVersion && m_versionTable->currentRow() > 0;
    m_publishBtn->setEnabled(hasSelectedVersion && !selectedPublished);
    m_unpublishBtn->setEnabled(hasSelectedVersion && selectedPublished);
    if (m_deleteVersionAction)
        m_deleteVersionAction->setEnabled(hasSelectedVersion);
    if (m_archiveAction)
        m_archiveAction->setEnabled(!m_currentStrategyId.isEmpty());
    if (m_moreActionsBtn)
        m_moreActionsBtn->setEnabled(hasSelectedVersion || !m_currentStrategyId.isEmpty());
    m_useInLiveBtn->setEnabled(hasSelectedVersion && selectedPublished);
    m_openBtBtn->setEnabled(hasSelectedVersion);
    m_diffToggle->setEnabled(hasDiffBase);
    updateVersionEditLock();
}

void StrategyDetailPanel::updateStrategyHeaderHint(const QJsonObject& catalogEntry)
{
    const QJsonObject lifecycleSummary =
        catalogEntry.value(QStringLiteral("lifecycleSummary")).toObject();
    const int publishedCount =
        lifecycleSummary.value(QStringLiteral("publishedVersionCount")).toInt();
    const bool liveActive =
        lifecycleSummary.value(QStringLiteral("liveDeploymentActive")).toBool();
    const QString lifecycle =
        lifecycleDisplayLabel(catalogEntry.value(QStringLiteral("lifecycleState")).toString());

    const QString publishedText =
        QStringLiteral("%1 published version%2")
            .arg(publishedCount)
            .arg(publishedCount == 1 ? QString() : QStringLiteral("s"));
    const QString liveText = liveActive
        ? QStringLiteral("Live deployment active")
        : QStringLiteral("not live");

    m_headerHintLabel->setText(
        QStringLiteral("Lifecycle: %1 - %2 - %3")
            .arg(lifecycle, publishedText, liveText));
}

bool StrategyDetailPanel::selectedVersionPublished() const
{
    if (!m_versionTable || !m_versionTable->selectionModel())
        return false;
    const QModelIndexList rows = m_versionTable->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return false;
    const int row = rows.first().row();
    if (row < 0 || row >= m_currentVersions.size())
        return false;
    return m_currentVersions[row].toObject().value("isPublished").toBool();
}

void StrategyDetailPanel::updateNewVersionButtonText()
{
    QString text = m_currentVersions.isEmpty()
        ? QStringLiteral("Save as First Version")
        : QStringLiteral("Save as New Version");
    if (m_configDirty)
        text += QStringLiteral(" *");
    m_newVersionBtn->setText(text);
}

void StrategyDetailPanel::updateVersionEditLock()
{
    const bool hasSelectedVersion = !selectedVersionId().isEmpty();
    const bool locked = hasSelectedVersion && selectedVersionPublished();

    if (m_policyEditor)
        m_policyEditor->setReadOnly(locked);
    if (m_inspector)
        m_inspector->setReadOnly(locked);

    if (m_versionEditStateLabel) {
        if (!hasSelectedVersion) {
            m_versionEditStateLabel->clear();
        } else if (locked) {
            m_versionEditStateLabel->setText(
                QStringLiteral("Parameters: locked for published version"));
        } else {
            m_versionEditStateLabel->setText(
                QStringLiteral("Parameters: editable"));
        }
    }

    if (m_newVersionBtn) {
        m_newVersionBtn->setToolTip(
            locked
                ? QStringLiteral("Create an unpublished version before changing parameters.")
                : QString());
    }
}

} // namespace StrategyMgmt
