#include "BlockWorkspace.h"
#include "WorkspaceHeader.h"
#include "LayoutConstants.h"
#include "cpipelinestrategyadapter.h"
#include "Pipeline/BlockRegistry.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>

BlockWorkspace::BlockWorkspace(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_header   = new WorkspaceHeader(this);
    m_tabWidget = new QTabWidget(this);

    layout->addWidget(m_header);
    layout->addWidget(m_tabWidget, 1);

    buildPropertiesTab();
    buildInfoTab();
    buildAssetsTab();
}

void BlockWorkspace::buildPropertiesTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_propertiesWidget = new QWidget(scroll);
    m_propertiesForm = new QFormLayout(m_propertiesWidget);
    m_propertiesForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                          Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_propertiesWidget);
    m_tabWidget->addTab(scroll, "Properties");
}

void BlockWorkspace::buildInfoTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_infoWidget = new QWidget(scroll);
    m_infoForm = new QFormLayout(m_infoWidget);
    m_infoForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                    Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_infoWidget);
    m_tabWidget->addTab(scroll, "Info");
}

void BlockWorkspace::buildAssetsTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_assetsWidget = new QWidget(scroll);
    m_assetsForm = new QFormLayout(m_assetsWidget);
    m_assetsForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                      Layout::TabContentMargin, Layout::TabContentMargin);

    auto* hint = new QLabel("Comma-separated list of symbols this block operates on.",
                             m_assetsWidget);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #888; font-size: 11px;");
    m_assetsForm->addRow(hint);

    m_symbolsEdit = new QLineEdit(m_assetsWidget);
    m_symbolsEdit->setPlaceholderText("e.g. AAPL, MSFT, GOOG");
    connect(m_symbolsEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_adapter) return;
        QJsonObject entry = currentBlockEntry();
        QJsonObject cfg = entry.value("config").toObject();

        QStringList symbols;
        for (const auto& s : m_symbolsEdit->text().split(','))
            if (!s.trimmed().isEmpty())
                symbols.append(s.trimmed());

        QJsonArray arr;
        for (const auto& s : symbols) arr.append(s);
        cfg["symbols"] = arr;
        entry["config"] = cfg;

        writeBackConfig();
    });
    m_assetsForm->addRow("Symbols", m_symbolsEdit);

    scroll->setWidget(m_assetsWidget);
    m_tabWidget->addTab(scroll, "Assets");
}

void BlockWorkspace::setBlockContext(CPipelineStrategyAdapter* adapter,
                                     const QString& category,
                                     const QString& blockId,
                                     const QString& jsonKey,
                                     int arrayIndex)
{
    m_adapter    = adapter;
    m_category   = category;
    m_blockId    = blockId;
    m_jsonKey    = jsonKey;
    m_arrayIndex = arrayIndex;

    m_header->setTitle(QStringLiteral("[%1] %2").arg(category, blockId));
    m_header->setBreadcrumb(adapter ? adapter->getName() : "");
    m_header->setState(DisplayState::Idle);

    bool isSelection = (category == "Selection");
    m_tabWidget->setTabVisible(m_tabWidget->indexOf(
        qobject_cast<QWidget*>(m_tabWidget->widget(2))), isSelection);

    refreshProperties();
    refreshInfo();
    if (isSelection)
        refreshAssets();

    m_tabWidget->setCurrentIndex(0);
}

void BlockWorkspace::clearContext()
{
    m_adapter = nullptr;
    m_header->clear();
    while (m_propertiesForm->rowCount() > 0) m_propertiesForm->removeRow(0);
    while (m_infoForm->rowCount() > 0) m_infoForm->removeRow(0);
    m_symbolsEdit->clear();
}

QJsonObject BlockWorkspace::currentBlockEntry() const
{
    if (!m_adapter) return {};
    const QJsonObject& cfg = m_adapter->pipelineConfig();
    QJsonValue val = cfg.value(m_jsonKey);

    if (val.isArray() && m_arrayIndex >= 0) {
        QJsonArray arr = val.toArray();
        if (m_arrayIndex < arr.size())
            return arr[m_arrayIndex].toObject();
    } else if (val.isObject()) {
        return val.toObject();
    }
    return {};
}

void BlockWorkspace::writeBackConfig()
{
    if (!m_adapter) return;

    QJsonObject entry = currentBlockEntry();
    QJsonObject cfg = entry.value("config").toObject();

    QStringList symbols;
    for (const auto& s : m_symbolsEdit->text().split(','))
        if (!s.trimmed().isEmpty())
            symbols.append(s.trimmed());
    QJsonArray symArr;
    for (const auto& s : symbols) symArr.append(s);
    cfg["symbols"] = symArr;
    entry["config"] = cfg;

    QJsonObject pipelineCfg = m_adapter->pipelineConfig();
    QJsonValue val = pipelineCfg.value(m_jsonKey);

    if (val.isArray() && m_arrayIndex >= 0) {
        QJsonArray arr = val.toArray();
        if (m_arrayIndex < arr.size())
            arr[m_arrayIndex] = entry;
        pipelineCfg[m_jsonKey] = arr;
    } else {
        pipelineCfg[m_jsonKey] = entry;
    }

    m_adapter->setPipelineConfig(pipelineCfg);
    emit configChanged(pipelineCfg);
}

void BlockWorkspace::refreshProperties()
{
    while (m_propertiesForm->rowCount() > 0)
        m_propertiesForm->removeRow(0);

    if (!m_adapter) return;

    QJsonObject entry = currentBlockEntry();
    QJsonObject cfg = entry.value("config").toObject();

    if (cfg.isEmpty()) {
        auto* empty = new QLabel("No configurable parameters.", m_propertiesWidget);
        empty->setStyleSheet("color: #888;");
        m_propertiesForm->addRow(empty);
        return;
    }

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

        auto* edit = new QLineEdit(value, m_propertiesWidget);
        connect(edit, &QLineEdit::editingFinished, this, [this, edit, key]() {
            if (!m_adapter) return;

            QJsonObject pipelineCfg = m_adapter->pipelineConfig();
            QJsonValue val = pipelineCfg.value(m_jsonKey);
            QJsonObject entry;

            if (val.isArray() && m_arrayIndex >= 0) {
                QJsonArray arr = val.toArray();
                if (m_arrayIndex < arr.size())
                    entry = arr[m_arrayIndex].toObject();
            } else if (val.isObject()) {
                entry = val.toObject();
            }

            QJsonObject cfg = entry.value("config").toObject();

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

            entry["config"] = cfg;

            if (val.isArray() && m_arrayIndex >= 0) {
                QJsonArray arr = val.toArray();
                if (m_arrayIndex < arr.size())
                    arr[m_arrayIndex] = entry;
                pipelineCfg[m_jsonKey] = arr;
            } else {
                pipelineCfg[m_jsonKey] = entry;
            }

            m_adapter->setPipelineConfig(pipelineCfg);
            emit configChanged(pipelineCfg);
        });

        m_propertiesForm->addRow(key, edit);
    }
}

void BlockWorkspace::refreshInfo()
{
    while (m_infoForm->rowCount() > 0)
        m_infoForm->removeRow(0);

    if (!m_adapter) return;

    auto addRow = [this](const QString& label, const QString& value) {
        auto* lbl = new QLabel(value, m_infoWidget);
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_infoForm->addRow(label, lbl);
    };

    addRow("Block ID", m_blockId);
    addRow("Category", m_category);
    addRow("JSON Key", m_jsonKey);
    if (m_arrayIndex >= 0)
        addRow("Array Index", QString::number(m_arrayIndex));
    addRow("Parent Strategy", m_adapter->getName());

    auto descResult = Pipeline::BlockRegistry::instance().descriptor(m_blockId);
    if (descResult) {
        const auto& desc = descResult.value();
        if (!desc.name.isEmpty())
            addRow("Display Name", desc.name);
        if (!desc.description.isEmpty())
            addRow("Description", desc.description);
        addRow("Scope", desc.scope == Pipeline::Scope::Strategy ? "Strategy" : "Portfolio");
    }
}

void BlockWorkspace::refreshAssets()
{
    if (!m_adapter) return;

    QJsonObject entry = currentBlockEntry();
    QJsonObject cfg = entry.value("config").toObject();
    QJsonArray symbols = cfg.value("symbols").toArray();

    QStringList symList;
    for (const auto& s : symbols)
        symList.append(s.toString());

    m_symbolsEdit->setText(symList.join(", "));
}
