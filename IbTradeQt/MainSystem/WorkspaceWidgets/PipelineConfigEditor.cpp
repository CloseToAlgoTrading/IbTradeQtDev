#include "PipelineConfigEditor.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"

#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHeaderView>

PipelineConfigEditor::PipelineConfigEditor(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void PipelineConfigEditor::buildUi()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    // Left: block tree
    m_blockTree = new QTreeWidget(m_splitter);
    m_blockTree->setHeaderLabels({QStringLiteral("Block"), QStringLiteral("Type")});
    m_blockTree->setColumnCount(2);
    m_blockTree->setRootIsDecorated(true);
    m_blockTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_blockTree->header()->setStretchLastSection(true);
    m_blockTree->setMinimumWidth(180);

    connect(m_blockTree, &QTreeWidget::currentItemChanged,
            this, [this]() { onBlockSelected(); });

    m_splitter->addWidget(m_blockTree);

    // Right: parameter panel
    m_paramScroll = new QScrollArea(m_splitter);
    m_paramScroll->setWidgetResizable(true);
    m_paramWidget = new QWidget(m_paramScroll);
    m_paramForm = new QFormLayout(m_paramWidget);
    m_paramForm->setContentsMargins(8, 8, 8, 8);
    m_paramScroll->setWidget(m_paramWidget);

    m_emptyLabel = new QLabel(QStringLiteral("Select a block to view its parameters."),
                              m_paramWidget);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #888; font-style: italic;"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_paramForm->addRow(m_emptyLabel);

    m_splitter->addWidget(m_paramScroll);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setSizes({200, 400});

    outerLayout->addWidget(m_splitter);
}

void PipelineConfigEditor::setPipelineConfig(const QJsonObject& config)
{
    m_config = config;
    rebuildTree();
    clearParamPanel();
}

void PipelineConfigEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

void PipelineConfigEditor::clear()
{
    m_config = QJsonObject();
    m_blockTree->clear();
    clearParamPanel();
}

// ---- tree construction ----

struct CategoryInfo {
    QString displayName;
    QLatin1StringView jsonKey;
    bool isArray;
};

static const CategoryInfo kCategories[] = {
    { QStringLiteral("Selection"),  Pipeline::Key::Selection, true  },
    { QStringLiteral("Alpha"),      Pipeline::Key::Alphas,    true  },
    { QStringLiteral("Risk"),       Pipeline::Key::Risks,     true  },
    { QStringLiteral("Rebalance"),  Pipeline::Key::Rebalance, false },
    { QStringLiteral("Execution"),  Pipeline::Key::Execution, false },
};

void PipelineConfigEditor::rebuildTree()
{
    m_blockTree->clear();

    for (const auto& cat : kCategories) {
        QJsonValue val = m_config.value(cat.jsonKey);

        QTreeWidgetItem* catItem = new QTreeWidgetItem(m_blockTree);
        catItem->setText(0, cat.displayName);
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
        QFont f = catItem->font(0);
        f.setBold(true);
        catItem->setFont(0, f);

        if (cat.isArray) {
            QJsonArray arr = val.toArray();
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject block = arr[i].toObject();
                QString blockId = block.value(Pipeline::Key::BlockId).toString();

                auto* blockItem = new QTreeWidgetItem(catItem);

                // Try to resolve a human-readable name from the registry
                auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
                blockItem->setText(0, desc ? desc.value().name : blockId);
                blockItem->setText(1, blockId);

                blockItem->setData(0, Qt::UserRole,     cat.displayName);
                blockItem->setData(0, Qt::UserRole + 1, QString(cat.jsonKey));
                blockItem->setData(0, Qt::UserRole + 2, true);
                blockItem->setData(0, Qt::UserRole + 3, i);
            }
        } else if (val.isObject() && !val.toObject().isEmpty()) {
            QJsonObject block = val.toObject();
            QString blockId = block.value(Pipeline::Key::BlockId).toString();

            auto* blockItem = new QTreeWidgetItem(catItem);
            auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
            blockItem->setText(0, desc ? desc.value().name : blockId);
            blockItem->setText(1, blockId);

            blockItem->setData(0, Qt::UserRole,     cat.displayName);
            blockItem->setData(0, Qt::UserRole + 1, QString(cat.jsonKey));
            blockItem->setData(0, Qt::UserRole + 2, false);
            blockItem->setData(0, Qt::UserRole + 3, 0);
        }

        catItem->setExpanded(true);
    }
}

// ---- parameter panel ----

void PipelineConfigEditor::clearParamPanel()
{
    while (m_paramForm->rowCount() > 0)
        m_paramForm->removeRow(0);

    m_emptyLabel = new QLabel(QStringLiteral("Select a block to view its parameters."),
                              m_paramWidget);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #888; font-style: italic;"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_paramForm->addRow(m_emptyLabel);
}

void PipelineConfigEditor::onBlockSelected()
{
    QTreeWidgetItem* item = m_blockTree->currentItem();
    if (!item || !item->parent()) {
        clearParamPanel();
        return;
    }

    QString category = item->data(0, Qt::UserRole).toString();
    QString jsonKey  = item->data(0, Qt::UserRole + 1).toString();
    bool isArray     = item->data(0, Qt::UserRole + 2).toBool();
    int arrayIndex   = item->data(0, Qt::UserRole + 3).toInt();

    showBlockParams(category, jsonKey, isArray, arrayIndex);
}

void PipelineConfigEditor::showBlockParams(const QString& category,
                                            const QString& jsonKey,
                                            bool isArray, int arrayIndex)
{
    while (m_paramForm->rowCount() > 0)
        m_paramForm->removeRow(0);

    // Locate the block entry
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

    // Header: block info
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

    // Separator
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
}
