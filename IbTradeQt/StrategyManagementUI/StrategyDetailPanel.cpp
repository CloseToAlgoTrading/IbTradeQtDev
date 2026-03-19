#include "StrategyDetailPanel.h"
#include "BlockInspectorPanel.h"

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

namespace StrategyMgmt {

StrategyDetailPanel::StrategyDetailPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyDetailPanel::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // --- Metadata section ---
    auto* metaGroup = new QGroupBox(QStringLiteral("Strategy Metadata"));
    auto* metaForm  = new QFormLayout(metaGroup);

    m_nameEdit = new QLineEdit;
    metaForm->addRow(QStringLiteral("Name:"), m_nameEdit);

    m_kindLabel = new QLabel;
    metaForm->addRow(QStringLiteral("Kind:"), m_kindLabel);

    m_statusCombo = new QComboBox;
    m_statusCombo->addItems({
        QStringLiteral("draft"),
        QStringLiteral("active"),
        QStringLiteral("testing"),
        QStringLiteral("retired")
    });
    metaForm->addRow(QStringLiteral("Status:"), m_statusCombo);

    m_descEdit = new QTextEdit;
    m_descEdit->setMaximumHeight(60);
    metaForm->addRow(QStringLiteral("Description:"), m_descEdit);

    m_tagsEdit = new QLineEdit;
    m_tagsEdit->setPlaceholderText(QStringLiteral("comma-separated tags"));
    metaForm->addRow(QStringLiteral("Tags:"), m_tagsEdit);

    m_saveMetaBtn = new QPushButton(QStringLiteral("Save Metadata"));
    metaForm->addRow(QString(), m_saveMetaBtn);

    mainLayout->addWidget(metaGroup);

    // --- Version table ---
    auto* verGroup = new QGroupBox(QStringLiteral("Versions"));
    auto* verLayout = new QVBoxLayout(verGroup);

    m_versionTable = new QTableWidget;
    m_versionTable->setColumnCount(4);
    m_versionTable->setHorizontalHeaderLabels({
        QStringLiteral("Version"),
        QStringLiteral("Published"),
        QStringLiteral("Notes"),
        QStringLiteral("Created At")
    });
    m_versionTable->horizontalHeader()->setStretchLastSection(true);
    m_versionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_versionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_versionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_versionTable->verticalHeader()->hide();
    m_versionTable->setMaximumHeight(120);
    verLayout->addWidget(m_versionTable);

    mainLayout->addWidget(verGroup);

    // --- Config tabs: Raw JSON + dynamic Block Details ---
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

    m_inspector = new BlockInspectorPanel;

    // Inspector param edits update working config
    connect(m_inspector, &BlockInspectorPanel::configChanged,
            this, [this](const QJsonObject& newCfg) {
        m_workingConfig = newCfg;
        markDirty();
    });

    mainLayout->addWidget(m_configTabs, 1);

    // --- Action buttons ---
    auto* actionBar = new QHBoxLayout;

    m_newVersionBtn = new QPushButton(QStringLiteral("Save as New Version"));
    m_publishBtn    = new QPushButton(QStringLiteral("Publish"));
    m_archiveBtn    = new QPushButton(QStringLiteral("Archive Strategy"));
    m_useInLiveBtn  = new QPushButton(QStringLiteral("Use in Live"));
    m_openBtBtn     = new QPushButton(QStringLiteral("Open in Backtest"));

    actionBar->addWidget(m_newVersionBtn);
    actionBar->addWidget(m_publishBtn);
    actionBar->addWidget(m_archiveBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_useInLiveBtn);
    actionBar->addWidget(m_openBtBtn);

    mainLayout->addLayout(actionBar);

    // --- Connections ---
    connect(m_versionTable, &QTableWidget::cellClicked,
            this, &StrategyDetailPanel::onVersionSelected);
    connect(m_saveMetaBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onSaveMetadata);
    connect(m_newVersionBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onNewVersion);
    connect(m_publishBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onPublish);
    connect(m_archiveBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onArchive);
    connect(m_useInLiveBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onUseInLive);
    connect(m_openBtBtn, &QPushButton::clicked,
            this, &StrategyDetailPanel::onOpenInBacktest);
    connect(m_diffToggle, &QCheckBox::toggled,
            this, [this](bool) {
        int row = m_versionTable->currentRow();
        if (row >= 0) onVersionSelected(row, 0);
    });

    clear();
}

void StrategyDetailPanel::showStrategy(const QJsonObject& catalogEntry,
                                        const QJsonArray& versions)
{
    m_currentStrategyId = catalogEntry.value("strategyId").toString();
    m_currentVersions   = versions;

    setEnabled(true);

    m_nameEdit->setText(catalogEntry.value("name").toString());
    int kind = catalogEntry.value("strategyKind").toInt();
    QString kindStr;
    switch (kind) {
    case 5:  kindStr = QStringLiteral("Pipeline"); break;
    case 1:  kindStr = QStringLiteral("Basic Test"); break;
    case 2:  kindStr = QStringLiteral("MA"); break;
    case 3:  kindStr = QStringLiteral("Momentum"); break;
    default: kindStr = QStringLiteral("Strategy"); break;
    }
    m_kindLabel->setText(kindStr);
    m_descEdit->setPlainText(catalogEntry.value("description").toString());
    m_tagsEdit->setText(catalogEntry.value("tags").toString());

    QString state = catalogEntry.value("lifecycleState").toString();
    int idx = m_statusCombo->findText(state);
    m_statusCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    m_versionTable->setRowCount(versions.size());
    for (int i = 0; i < versions.size(); ++i) {
        QJsonObject v = versions[i].toObject();
        m_versionTable->setItem(i, 0, new QTableWidgetItem(
            QStringLiteral("v%1").arg(v.value("versionNumber").toInt())));
        m_versionTable->setItem(i, 1, new QTableWidgetItem(
            v.value("isPublished").toBool() ? QStringLiteral("Yes") : QStringLiteral("No")));
        m_versionTable->setItem(i, 2, new QTableWidgetItem(
            v.value("notes").toString()));
        m_versionTable->setItem(i, 3, new QTableWidgetItem(
            v.value("createdAt").toString()));
    }

    m_configViewer->clear();
    hideBlockDetails();

    if (versions.size() > 0) {
        QJsonObject latest = versions.last().toObject();
        QString cfgStr = latest.value("configJson").toString();
        m_workingConfig = QJsonDocument::fromJson(cfgStr.toUtf8()).object();
        m_configDirty = false;
        m_newVersionBtn->setText(QStringLiteral("Save as New Version"));

        m_versionTable->selectRow(versions.size() - 1);
        onVersionSelected(versions.size() - 1, 0);
    } else {
        m_workingConfig = QJsonObject();
        m_configDirty = false;
    }
}

void StrategyDetailPanel::clear()
{
    m_currentStrategyId.clear();
    m_currentVersions = QJsonArray();
    m_nameEdit->clear();
    m_kindLabel->clear();
    m_descEdit->clear();
    m_tagsEdit->clear();
    m_statusCombo->setCurrentIndex(0);
    m_versionTable->setRowCount(0);
    m_configViewer->clear();
    hideBlockDetails();
    m_workingConfig = QJsonObject();
    m_configDirty = false;
    setEnabled(false);
}

void StrategyDetailPanel::showBlockDetails(const QString& category,
                                            const QString& jsonKey,
                                            bool isArray, int arrayIndex)
{
    if (m_workingConfig.isEmpty()) return;

    if (m_inspectorTabIdx < 0) {
        m_inspectorTabIdx = m_configTabs->addTab(m_inspector, QStringLiteral("Block Details"));
    }
    m_configTabs->setCurrentIndex(m_inspectorTabIdx);
    m_inspector->showBlock(m_workingConfig, category, jsonKey, isArray, arrayIndex);
}

void StrategyDetailPanel::hideBlockDetails()
{
    if (m_inspectorTabIdx >= 0) {
        m_configTabs->removeTab(m_inspectorTabIdx);
        m_inspectorTabIdx = -1;
    }
}

void StrategyDetailPanel::onVersionSelected(int row, int)
{
    if (row < 0 || row >= m_currentVersions.size()) {
        m_configViewer->clear();
        return;
    }

    QJsonObject v = m_currentVersions[row].toObject();
    QString configJson = v.value("configJson").toString();
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());

    m_workingConfig = doc.object();
    m_configDirty = false;
    m_newVersionBtn->setText(QStringLiteral("Save as New Version"));
    hideBlockDetails();

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
}

void StrategyDetailPanel::markDirty()
{
    m_configDirty = true;
    m_newVersionBtn->setText(QStringLiteral("Save as New Version *"));

    QJsonDocument doc(m_workingConfig);
    m_configViewer->setPlainText(doc.toJson(QJsonDocument::Indented));
}

// --- Metadata & actions ---

void StrategyDetailPanel::onSaveMetadata()
{
    if (m_currentStrategyId.isEmpty()) return;
    emit metadataChanged(m_currentStrategyId,
                         m_nameEdit->text(),
                         m_descEdit->toPlainText(),
                         m_tagsEdit->text(),
                         m_statusCombo->currentText());
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
    int row = m_versionTable->currentRow();
    if (row < 0 || row >= m_currentVersions.size())
        return {};
    return m_currentVersions[row].toObject().value("versionId").toString();
}

} // namespace StrategyMgmt
