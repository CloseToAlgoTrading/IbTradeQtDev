#include "BlockInspectorPanel.h"
#include "BlockInspectorPresenter.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonDocument>

BlockInspectorPanel::BlockInspectorPanel(QWidget* parent)
    : QWidget(parent)
{
    m_presenter = new BlockInspectorPresenter(this);
    buildUi();
}

void BlockInspectorPanel::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_paramScroll = new QScrollArea(this);
    m_paramScroll->setWidgetResizable(true);
    m_paramWidget = new QWidget(m_paramScroll);
    m_paramForm = new QFormLayout(m_paramWidget);
    m_paramForm->setContentsMargins(8, 8, 8, 8);
    m_paramScroll->setWidget(m_paramWidget);

    layout->addWidget(m_paramScroll, 3);

    // JSON diff panel (below the inspector, hidden by default)
    m_diffContainer = new QWidget(this);
    auto* diffLayout = new QVBoxLayout(m_diffContainer);
    diffLayout->setContentsMargins(4, 4, 4, 4);
    diffLayout->setSpacing(2);

    m_diffToggle = new QCheckBox(QStringLiteral("Diff vs Baseline"), m_diffContainer);
    diffLayout->addWidget(m_diffToggle);

    m_jsonViewer = new QPlainTextEdit(m_diffContainer);
    m_jsonViewer->setReadOnly(true);
    diffLayout->addWidget(m_jsonViewer);

    m_diffContainer->setVisible(false);
    layout->addWidget(m_diffContainer, 2);

    m_emptyLabel = new QLabel(QStringLiteral("Select a block to view its parameters."),
                              m_paramWidget);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #888; font-style: italic;"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_paramForm->addRow(m_emptyLabel);

    connect(m_diffToggle, &QCheckBox::toggled, this, [this](bool) { updateJsonViewer(); });
}

// ---- Configuration ----

void BlockInspectorPanel::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

void BlockInspectorPanel::setDiffPanelVisible(bool visible)
{
    m_diffContainer->setVisible(visible);
}

void BlockInspectorPanel::setComparisonConfig(const QJsonObject& baseline)
{
    m_comparisonConfig = baseline;
    updateJsonViewer();
}

void BlockInspectorPanel::clear()
{
    m_config = QJsonObject();
    m_comparisonConfig = QJsonObject();
    clearForm();
    if (m_jsonViewer) m_jsonViewer->clear();
}

void BlockInspectorPanel::clearForm()
{
    while (m_paramForm->rowCount() > 0)
        m_paramForm->removeRow(0);

    m_emptyLabel = new QLabel(QStringLiteral("Select a block to view its parameters."),
                              m_paramWidget);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #888; font-style: italic;"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_paramForm->addRow(m_emptyLabel);
}

// ---- Block display ----

void BlockInspectorPanel::showBlock(const QJsonObject& pipelineConfig,
                                     const QString& category,
                                     const QString& jsonKey,
                                     bool isArray, int arrayIndex)
{
    m_config = pipelineConfig;

    while (m_paramForm->rowCount() > 0)
        m_paramForm->removeRow(0);

    QJsonValue val = m_config.value(jsonKey);
    QJsonObject entry;
    if (isArray) {
        QJsonArray arr = val.toArray();
        if (arrayIndex >= 0 && arrayIndex < arr.size())
            entry = arr[arrayIndex].toObject();
    } else {
        entry = val.toObject();
    }

    QString blockId = entry.value(Pipeline::Key::BlockId).toString();
    QJsonObject cfg = entry.value(Pipeline::Key::Config).toObject();

    auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
    QString displayName = desc ? desc.value().name : blockId;
    QString description = desc ? desc.value().description : QString();

    auto* headerLabel = new QLabel(
        QStringLiteral("<b>%1</b> <span style='color:#888;'>(%2)</span>")
            .arg(displayName, category),
        m_paramWidget);
    m_paramForm->addRow(headerLabel);

    if (!description.isEmpty()) {
        auto* descLabel = new QLabel(description, m_paramWidget);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 11px; margin-bottom: 8px;"));
        m_paramForm->addRow(descLabel);
    }

    auto* sep = new QLabel(QStringLiteral("<b>Parameters</b>"), m_paramWidget);
    m_paramForm->addRow(sep);

    if (cfg.isEmpty()) {
        auto* noParams = new QLabel(QStringLiteral("No configurable parameters."), m_paramWidget);
        noParams->setStyleSheet(QStringLiteral("color: #888; font-style: italic;"));
        m_paramForm->addRow(noParams);
    } else {
        for (auto it = cfg.begin(); it != cfg.end(); ++it) {
            QString key = it.key();
            QString value;

            if (it.value().isArray()) {
                QJsonDocument doc(it.value().toArray());
                value = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
            } else if (it.value().isObject()) {
                QJsonDocument doc(it.value().toObject());
                value = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
            } else {
                value = it.value().toVariant().toString();
            }

            auto* edit = new QLineEdit(value, m_paramWidget);
            edit->setReadOnly(m_readOnly);

            if (!m_readOnly) {
                connect(edit, &QLineEdit::editingFinished, this,
                        [this, jsonKey, isArray, arrayIndex, key, edit]() {
                    QJsonValue val = m_config.value(jsonKey);
                    QJsonObject entry;

                    if (isArray) {
                        QJsonArray arr = val.toArray();
                        if (arrayIndex >= 0 && arrayIndex < arr.size())
                            entry = arr[arrayIndex].toObject();
                    } else {
                        entry = val.toObject();
                    }

                    QJsonObject cfg = entry.value(Pipeline::Key::Config).toObject();

                    QString text = edit->text();
                    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
                    if (!doc.isNull()) {
                        if (doc.isArray())
                            cfg[key] = doc.array();
                        else if (doc.isObject())
                            cfg[key] = doc.object();
                    } else {
                        bool ok;
                        double num = text.toDouble(&ok);
                        if (ok)
                            cfg[key] = num;
                        else
                            cfg[key] = text;
                    }

                    entry[Pipeline::Key::Config] = cfg;

                    if (isArray) {
                        QJsonArray arr = m_config.value(jsonKey).toArray();
                        if (arrayIndex >= 0 && arrayIndex < arr.size())
                            arr[arrayIndex] = entry;
                        m_config[jsonKey] = arr;
                    } else {
                        m_config[jsonKey] = entry;
                    }

                    emit configChanged(m_config);
                    updateJsonViewer();
                });
            }

            m_paramForm->addRow(key, edit);
        }
    }

    // Block metadata section
    auto* infoSep = new QLabel(QStringLiteral("<b>Block Info</b>"), m_paramWidget);
    infoSep->setStyleSheet(QStringLiteral("margin-top: 12px;"));
    m_paramForm->addRow(infoSep);

    auto addInfoRow = [this](const QString& label, const QString& value) {
        auto* lbl = new QLabel(value, m_paramWidget);
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lbl->setStyleSheet(QStringLiteral("color: #aaa;"));
        m_paramForm->addRow(label, lbl);
    };

    addInfoRow(QStringLiteral("Block ID"), blockId);
    addInfoRow(QStringLiteral("Category"), category);
    if (desc) {
        addInfoRow(QStringLiteral("Scope"),
                   desc.value().scope == Pipeline::Scope::Strategy
                       ? QStringLiteral("Strategy")
                       : QStringLiteral("Portfolio"));
    }

    updateJsonViewer();
}

// ---- JSON diff ----

void BlockInspectorPanel::updateJsonViewer()
{
    if (!m_jsonViewer || !m_diffContainer->isVisible())
        return;

    QJsonDocument curDoc(m_config);
    QString currentText = curDoc.toJson(QJsonDocument::Indented);

    if (m_diffToggle->isChecked() && !m_comparisonConfig.isEmpty()) {
        QJsonDocument baseDoc(m_comparisonConfig);
        QString baselineText = baseDoc.toJson(QJsonDocument::Indented);
        m_jsonViewer->setPlainText(computeDiff(currentText, baselineText));
    } else {
        m_jsonViewer->setPlainText(currentText);
    }
}

QString BlockInspectorPanel::computeDiff(const QString& current, const QString& baseline) const
{
    QStringList curLines  = current.split('\n');
    QStringList baseLines = baseline.split('\n');

    QString diff;
    int maxLines = qMax(curLines.size(), baseLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString cl = (i < curLines.size())  ? curLines[i]  : QString();
        QString bl = (i < baseLines.size()) ? baseLines[i] : QString();
        if (cl == bl) {
            diff += QStringLiteral("  ") + cl + '\n';
        } else {
            if (!bl.isEmpty())
                diff += QStringLiteral("- ") + bl + '\n';
            if (!cl.isEmpty())
                diff += QStringLiteral("+ ") + cl + '\n';
        }
    }
    return diff;
}
