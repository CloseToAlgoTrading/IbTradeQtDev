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
#include <QStackedWidget>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QMenu>
#include <QDateTime>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QPainter>

namespace StrategyMgmt {

class LifecycleComboBox : public QComboBox
{
public:
    explicit LifecycleComboBox(QWidget* parent = nullptr)
        : QComboBox(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(palette().color(QPalette::Text));
        painter.setFont(font());
        painter.drawText(rect().adjusted(4, 0, 0, 0),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         currentText());
    }
};

static QString versionAvailabilityLabel(const QJsonObject& version)
{
    return version.value(QStringLiteral("isPublished")).toBool()
        ? QStringLiteral("Published")
        : QStringLiteral("Unpublished");
}

static QString lifecycleLabel(const QString& lifecycleState)
{
    const QString state = lifecycleState.trimmed().toLower();
    if (state == QStringLiteral("testing"))
        return QStringLiteral("Testing");
    if (state == QStringLiteral("ready"))
        return QStringLiteral("Ready");
    if (state == QStringLiteral("retired"))
        return QStringLiteral("Retired");
    return QStringLiteral("Draft");
}

static QColor lifecycleColor(const QString& lifecycleState)
{
    const QString state = lifecycleState.trimmed().toLower();
    if (state == QStringLiteral("testing"))
        return QColor(QStringLiteral("#d4a04a"));
    if (state == QStringLiteral("ready"))
        return QColor(QStringLiteral("#58a6ff"));
    if (state == QStringLiteral("retired"))
        return QColor(QStringLiteral("#888888"));
    return QColor(QStringLiteral("#aaaaaa"));
}

static QString lifecyclePillStyle(const QString& lifecycleState, bool editable)
{
    const QColor accent = lifecycleColor(lifecycleState);
    const QString text = accent.lighter(116).name();

    if (!editable) {
        return QStringLiteral(
            "QLabel {"
            " background-color:transparent;"
            " color:%1;"
            " border:none;"
            " padding:0 4px;"
            " font-size:11px;"
            " min-height:16px;"
            "}").arg(text);
    }

    return QStringLiteral(
        "QComboBox {"
        " background-color:transparent;"
        " color:%1;"
        " border:none;"
        " padding:0;"
        " margin:0;"
        " font-size:11px;"
        " min-height:0;"
        " selection-background-color:#2a3a50;"
        "}"
        "QComboBox:hover { background-color:transparent; border:none; }"
        "QComboBox:focus, QComboBox:on { background-color:transparent; border:none; }"
        "QComboBox::drop-down { border:none; width:0; }"
        "QComboBox::down-arrow { image:none; width:0; height:0; }"
        "QComboBox QAbstractItemView {"
        " background-color:#1f1f1f;"
        " color:#e8e8e8;"
        " border:1px solid #3a3a3a;"
        " selection-background-color:#2a3a50;"
        " selection-color:#ffffff;"
        " outline:0;"
        " padding:2px;"
        "}").arg(text);
}

static void applyLifecycleBadgeStyle(QLabel* label, const QString& lifecycleState)
{
    if (!label)
        return;
    label->setText(lifecycleLabel(lifecycleState));
    label->setStyleSheet(lifecyclePillStyle(lifecycleState, false));
}

static void applyLifecycleComboStyle(QComboBox* combo)
{
    if (!combo)
        return;
    combo->setStyleSheet(lifecyclePillStyle(combo->currentData().toString(), true));
}

static QString displayTimestamp(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QDateTime dt = QDateTime::fromString(trimmed, Qt::ISODate);
    if (dt.isValid())
        return dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    return trimmed;
}

static QString versionCreatedText(const QJsonObject& version,
                                  const QJsonObject& catalogEntry)
{
    QString text = displayTimestamp(version.value(QStringLiteral("createdAt")).toString());
    if (!text.isEmpty())
        return text;
    text = displayTimestamp(version.value(QStringLiteral("created_at")).toString());
    if (!text.isEmpty())
        return text;
    text = displayTimestamp(catalogEntry.value(QStringLiteral("updatedAt")).toString());
    if (!text.isEmpty())
        return text;
    text = displayTimestamp(catalogEntry.value(QStringLiteral("createdAt")).toString());
    return text.isEmpty() ? QStringLiteral("Not recorded") : text;
}

static QComboBox* createLifecycleCombo(const QString& lifecycleState)
{
    auto* combo = new LifecycleComboBox;
    combo->addItem(QStringLiteral("Draft"), QStringLiteral("draft"));
    combo->addItem(QStringLiteral("Testing"), QStringLiteral("testing"));
    combo->addItem(QStringLiteral("Ready"), QStringLiteral("ready"));
    combo->addItem(QStringLiteral("Retired"), QStringLiteral("retired"));
    const int idx = combo->findData(lifecycleState.trimmed().toLower());
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
    combo->setObjectName(QStringLiteral("VersionLifecycleCombo"));
    combo->setCursor(Qt::PointingHandCursor);
    combo->setContentsMargins(0, 0, 0, 0);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    if (combo->view())
        combo->view()->setMinimumWidth(104);
    applyLifecycleComboStyle(combo);
    return combo;
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

    auto* titleRow = new QHBoxLayout;
    m_nameEdit = new QLineEdit;
    m_nameEdit->setObjectName(QStringLiteral("StrategyTitleEdit"));
    m_lifecycleBadge = new QLabel;
    m_lifecycleBadge->setObjectName(QStringLiteral("StrategyLifecycleBadge"));
    m_lifecycleBadge->setAlignment(Qt::AlignCenter);

    titleRow->addWidget(m_nameEdit, 1);
    titleRow->addWidget(m_lifecycleBadge, 0, Qt::AlignVCenter);
    headerLayout->addLayout(titleRow);

    auto* metaForm = new QFormLayout;
    metaForm->setContentsMargins(0, 0, 0, 0);

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
    m_versionTable->setColumnCount(5);
    m_versionTable->setHorizontalHeaderLabels({
        QStringLiteral("Version"),
        QStringLiteral("Published"),
        QStringLiteral("Notes"),
        QStringLiteral("Lifecycle"),
        QStringLiteral("Created")
    });
    m_versionTable->horizontalHeader()->setStretchLastSection(false);
    m_versionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_versionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_versionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_versionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_versionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_versionTable->setColumnWidth(0, 76);
    m_versionTable->setColumnWidth(1, 104);
    m_versionTable->setColumnWidth(3, 112);
    m_versionTable->setColumnWidth(4, 138);
    m_versionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_versionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_versionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_versionTable->verticalHeader()->hide();
    m_versionTable->setMaximumHeight(120);
    verLayout->addWidget(m_versionTable);

    mainLayout->addWidget(verGroup);

    // --- Scope inspector ---
    auto* inspectorGroup = new QGroupBox(QStringLiteral("Inspector"));
    auto* inspectorLayout = new QVBoxLayout(inspectorGroup);
    inspectorLayout->setContentsMargins(8, 8, 8, 8);
    inspectorLayout->setSpacing(6);

    m_inspectorTitleLabel = new QLabel;
    m_inspectorTitleLabel->setObjectName(QStringLiteral("StrategyInspectorTitleLabel"));
    inspectorLayout->addWidget(m_inspectorTitleLabel);

    m_inspectorStack = new QStackedWidget;

    m_versionInspectorPage = new QWidget;
    auto* versionInspectorLayout = new QVBoxLayout(m_versionInspectorPage);
    versionInspectorLayout->setContentsMargins(0, 0, 0, 0);
    versionInspectorLayout->setSpacing(6);

    m_policyEditor = new RuntimePolicyEditor;
    auto* policyScroll = new QScrollArea;
    policyScroll->setWidgetResizable(true);
    policyScroll->setFrameShape(QFrame::NoFrame);
    policyScroll->setWidget(m_policyEditor);
    versionInspectorLayout->addWidget(policyScroll, 1);

    m_showAdvancedJsonCheck = new QCheckBox(QStringLiteral("Show advanced configuration"));
    versionInspectorLayout->addWidget(m_showAdvancedJsonCheck);

    m_jsonPanel = new QWidget;
    auto* jsonLayout = new QVBoxLayout(m_jsonPanel);
    jsonLayout->setContentsMargins(0, 0, 0, 0);
    jsonLayout->setSpacing(4);

    m_diffToggle = new QCheckBox(QStringLiteral("Compare with previous version"));
    jsonLayout->addWidget(m_diffToggle);

    m_configViewer = new QPlainTextEdit;
    m_configViewer->setReadOnly(true);
    jsonLayout->addWidget(m_configViewer);
    m_jsonPanel->setVisible(false);
    versionInspectorLayout->addWidget(m_jsonPanel, 1);

    m_inspectorStack->addWidget(m_versionInspectorPage);

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

    m_blockInspectorPage = new QWidget;
    auto* blockInspectorLayout = new QVBoxLayout(m_blockInspectorPage);
    blockInspectorLayout->setContentsMargins(0, 0, 0, 0);

    m_inspector = new BlockInspectorPanel;
    auto* inspectorScroll = new QScrollArea;
    inspectorScroll->setWidgetResizable(true);
    inspectorScroll->setFrameShape(QFrame::NoFrame);
    inspectorScroll->setWidget(m_inspector);
    blockInspectorLayout->addWidget(inspectorScroll);
    m_inspectorStack->addWidget(m_blockInspectorPage);

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

    inspectorLayout->addWidget(m_inspectorStack, 1);
    mainLayout->addWidget(inspectorGroup, 1);

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

    mainLayout->insertLayout(2, actionBar);

    // --- Connections ---
    connect(m_versionTable, &QTableWidget::currentCellChanged,
            this, &StrategyDetailPanel::onVersionCurrentCellChanged);
    connect(m_nameEdit, &QLineEdit::editingFinished,
            this, &StrategyDetailPanel::onSaveMetadata);
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
    connect(m_showAdvancedJsonCheck, &QCheckBox::toggled,
            m_jsonPanel, &QWidget::setVisible);

    clear();
}

void StrategyDetailPanel::showStrategy(const QJsonObject& catalogEntry,
                                        const QJsonArray& versions)
{
    m_currentStrategyId = catalogEntry.value("strategyId").toString();
    m_currentVersions   = versions;

    setEnabled(true);

    const QString strategyName = catalogEntry.value("name").toString();
    m_currentStrategyLifecycle = catalogEntry.value(QStringLiteral("lifecycleState"))
        .toString(QStringLiteral("draft"));
    m_nameEdit->setText(strategyName);
    applyLifecycleBadgeStyle(m_lifecycleBadge, m_currentStrategyLifecycle);
    m_descEdit->setPlainText(catalogEntry.value("description").toString());
    m_tagsEdit->setText(catalogEntry.value("tags").toString());

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

        auto* lifecycleCombo =
            createLifecycleCombo(v.value(QStringLiteral("lifecycleState")).toString(
                m_currentStrategyLifecycle));
        lifecycleCombo->setProperty("versionId", v.value(QStringLiteral("versionId")).toString());
        connect(lifecycleCombo, &QComboBox::currentIndexChanged,
                this, [this, lifecycleCombo](int) {
            applyLifecycleComboStyle(lifecycleCombo);
            const QString versionId = lifecycleCombo->property("versionId").toString();
            const QString lifecycleState = lifecycleCombo->currentData().toString();
            if (!m_currentStrategyId.isEmpty() && !versionId.isEmpty())
                emit versionLifecycleChanged(m_currentStrategyId, versionId, lifecycleState);
        });
        m_versionTable->setCellWidget(i, 3, lifecycleCombo);

        m_versionTable->setItem(i, 4, new QTableWidgetItem(
            versionCreatedText(v, catalogEntry)));
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
        emit visibleVersionChanged(m_currentStrategyId,
                                   QString(),
                                   QString(),
                                   QString(),
                                   QJsonObject());
    }
    showVersionInspector();
}

void StrategyDetailPanel::clear()
{
    const QString previousStrategyId = m_currentStrategyId;
    m_currentStrategyId.clear();
    m_currentStrategyLifecycle = QStringLiteral("draft");
    m_blockInspectorTitle.clear();
    m_currentVersions = QJsonArray();
    m_nameEdit->clear();
    if (m_lifecycleBadge)
        m_lifecycleBadge->clear();
    m_descEdit->clear();
    m_tagsEdit->clear();
    m_versionTable->setRowCount(0);
    m_configViewer->clear();
    if (m_showAdvancedJsonCheck) {
        QSignalBlocker blocker(m_showAdvancedJsonCheck);
        m_showAdvancedJsonCheck->setChecked(false);
    }
    if (m_jsonPanel)
        m_jsonPanel->setVisible(false);
    hideBlockDetails();
    m_workingConfig = QJsonObject();
    m_configDirty = false;
    updateNewVersionButtonText();
    updateVersionActionState();
    emit visibleVersionChanged(previousStrategyId,
                               QString(),
                               QString(),
                               QString(),
                               QJsonObject());
    setEnabled(false);
}

void StrategyDetailPanel::setSelectedBlockContext(const QString& category,
                                                  const QString& blockName)
{
    if (category.isEmpty() || blockName.isEmpty()) {
        m_blockInspectorTitle.clear();
        return;
    }
    m_blockInspectorTitle = QStringLiteral("Block Details: %1").arg(blockName);
}

void StrategyDetailPanel::showBlockDetails(const QString& category,
                                            const QString& jsonKey,
                                            bool isArray, int arrayIndex)
{
    if (m_workingConfig.isEmpty()) return;

    if (m_inspectorTitleLabel) {
        m_inspectorTitleLabel->setText(
            m_blockInspectorTitle.isEmpty()
                ? QStringLiteral("Block Details")
                : m_blockInspectorTitle);
    }
    if (m_inspectorStack && m_blockInspectorPage)
        m_inspectorStack->setCurrentWidget(m_blockInspectorPage);
    m_inspector->setReadOnly(currentVersionPublished());
    m_inspector->showBlock(m_workingConfig, category, jsonKey, isArray, arrayIndex);
}

void StrategyDetailPanel::hideBlockDetails()
{
    m_inspector->clear();
    m_blockInspectorTitle.clear();
    showVersionInspector();
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
    showVersionInspector();

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
    emit visibleVersionChanged(m_currentStrategyId,
                               versionIdForRow(row),
                               versionLabelForRow(row),
                               v.value(QStringLiteral("lifecycleState")).toString(
                                   m_currentStrategyLifecycle),
                               m_workingConfig);
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
    showVersionInspector();
    markDirty();
}

void StrategyDetailPanel::markDirty()
{
    m_configDirty = true;
    updateNewVersionButtonText();

    QJsonDocument doc(m_workingConfig);
    m_configViewer->setPlainText(doc.toJson(QJsonDocument::Indented));
    updateVersionActionState();
    const int row = m_versionTable ? m_versionTable->currentRow() : -1;
    QString label = versionLabelForRow(row);
    if (!label.isEmpty())
        label += QStringLiteral(" *");
    const QJsonObject version = row >= 0 && row < m_currentVersions.size()
        ? m_currentVersions[row].toObject()
        : QJsonObject();
    emit visibleVersionChanged(m_currentStrategyId,
                               versionIdForRow(row),
                               label,
                               version.value(QStringLiteral("lifecycleState")).toString(
                                   m_currentStrategyLifecycle),
                               m_workingConfig);
}

// --- Metadata & actions ---

void StrategyDetailPanel::onSaveMetadata()
{
    if (m_currentStrategyId.isEmpty()) return;
    emit metadataChanged(m_currentStrategyId,
                         m_nameEdit->text().trimmed(),
                         m_descEdit->toPlainText(),
                         m_tagsEdit->text(),
                         m_currentStrategyLifecycle);
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

QString StrategyDetailPanel::selectedVersionLabel() const
{
    if (!m_versionTable || !m_versionTable->selectionModel())
        return {};
    const QModelIndexList rows = m_versionTable->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return {};
    const int row = rows.first().row();
    if (row < 0 || row >= m_currentVersions.size())
        return {};
    return versionLabelForRow(row);
}

QString StrategyDetailPanel::versionIdForRow(int row) const
{
    if (row < 0 || row >= m_currentVersions.size())
        return {};
    return m_currentVersions[row].toObject().value(QStringLiteral("versionId")).toString();
}

QString StrategyDetailPanel::versionLabelForRow(int row) const
{
    if (row < 0 || row >= m_currentVersions.size())
        return {};
    const int versionNumber =
        m_currentVersions[row].toObject().value(QStringLiteral("versionNumber")).toInt();
    return versionNumber > 0 ? QStringLiteral("v%1").arg(versionNumber) : QString();
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

void StrategyDetailPanel::showVersionInspector()
{
    QString title = QStringLiteral("Version Details");
    const QString versionLabel = selectedVersionLabel();
    if (!versionLabel.isEmpty())
        title += QStringLiteral(": %1").arg(versionLabel);

    if (m_inspectorTitleLabel)
        m_inspectorTitleLabel->setText(title);
    if (m_inspectorStack && m_versionInspectorPage)
        m_inspectorStack->setCurrentWidget(m_versionInspectorPage);
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

    if (m_newVersionBtn) {
        m_newVersionBtn->setToolTip(
            locked
                ? QStringLiteral("Create an unpublished version before changing parameters.")
                : QString());
    }
}

} // namespace StrategyMgmt
