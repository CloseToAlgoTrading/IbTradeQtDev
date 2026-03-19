#include "StrategyCatalogPanel.h"
#include "PipelineTreeUtils.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>

namespace StrategyMgmt {

static QString kindLabel(int k) {
    switch (k) {
    case 0: return QStringLiteral("pipeline");
    case 1: return QStringLiteral("classic");
    default: return QStringLiteral("unknown");
    }
}

StrategyCatalogPanel::StrategyCatalogPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyCatalogPanel::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* topBar = new QHBoxLayout;

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("Search strategies..."));
    m_searchEdit->setClearButtonEnabled(true);
    topBar->addWidget(m_searchEdit, 1);

    m_statusCombo = new QComboBox;
    m_statusCombo->addItem(QStringLiteral("All"));
    m_statusCombo->addItem(QStringLiteral("draft"));
    m_statusCombo->addItem(QStringLiteral("active"));
    m_statusCombo->addItem(QStringLiteral("archived"));
    topBar->addWidget(m_statusCombo);

    m_newButton = new QPushButton(QStringLiteral("New Strategy"));
    topBar->addWidget(m_newButton);
    layout->addLayout(topBar);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({QStringLiteral("Name"),
                             QStringLiteral("Kind"),
                             QStringLiteral("Status")});
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->header()->setStretchLastSection(true);
    layout->addWidget(m_tree, 1);

    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &StrategyCatalogPanel::onFilterChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StrategyCatalogPanel::onStatusFilterChanged);
    connect(m_newButton, &QPushButton::clicked,
            this, &StrategyCatalogPanel::newStrategyRequested);
    connect(m_tree, &QTreeWidget::itemClicked,
            this, &StrategyCatalogPanel::onItemClicked);
}

void StrategyCatalogPanel::populate(const QJsonArray& catalogEntries,
                                     const QMap<QString, int>& versionCounts,
                                     const QMap<QString, QJsonObject>& latestConfigs)
{
    m_entries.clear();
    for (const auto& val : catalogEntries) {
        QJsonObject obj = val.toObject();
        CatalogEntry e;
        e.strategyId     = obj.value(QStringLiteral("strategyId")).toString();
        e.name           = obj.value(QStringLiteral("name")).toString();
        e.strategyKind   = obj.value(QStringLiteral("strategyKind")).toInt();
        e.lifecycleState = obj.value(QStringLiteral("lifecycleState")).toString();
        e.versionCount   = versionCounts.value(e.strategyId, 0);
        e.updatedAt      = obj.value(QStringLiteral("updatedAt")).toString();
        e.pipelineConfig = latestConfigs.value(e.strategyId);
        m_entries.append(e);
    }
    applyFilter();
}

void StrategyCatalogPanel::applyFilter()
{
    m_tree->clear();

    const QString lf = m_searchEdit->text().toLower();
    const QString statusFilter = (m_statusCombo->currentIndex() == 0)
                                     ? QString()
                                     : m_statusCombo->currentText();

    for (const auto& e : m_entries) {
        if (!lf.isEmpty() && !e.name.toLower().contains(lf))
            continue;
        if (!statusFilter.isEmpty() && e.lifecycleState != statusFilter)
            continue;

        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, e.name);
        item->setText(1, kindLabel(e.strategyKind));
        item->setText(2, e.lifecycleState);

        item->setData(0, RoleStrategyId, e.strategyId);
        item->setData(0, RoleIsStrategy, true);
        item->setData(0, RoleStatus,     e.lifecycleState);

        QFont f = item->font(0);
        f.setBold(true);
        item->setFont(0, f);

        if (!e.pipelineConfig.isEmpty())
            PipelineTreeUtils::populateBlockNodes(item, e.pipelineConfig);
    }

    m_tree->expandAll();
}

void StrategyCatalogPanel::onItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;

    // Block leaf?
    if (item->data(0, PipelineTreeUtils::RoleIsBlock).toBool()) {
        // Walk up to strategy ancestor
        QTreeWidgetItem* ancestor = item->parent();
        while (ancestor && !ancestor->data(0, RoleIsStrategy).toBool())
            ancestor = ancestor->parent();

        QString strategyId = ancestor ? ancestor->data(0, RoleStrategyId).toString() : QString();

        emit blockSelected(strategyId,
                           item->data(0, PipelineTreeUtils::RoleCategory).toString(),
                           item->data(0, PipelineTreeUtils::RoleJsonKey).toString(),
                           item->data(0, PipelineTreeUtils::RoleIsArray).toBool(),
                           item->data(0, PipelineTreeUtils::RoleArrayIndex).toInt());
        return;
    }

    // Strategy row?
    if (item->data(0, RoleIsStrategy).toBool()) {
        QString sid = item->data(0, RoleStrategyId).toString();
        if (!sid.isEmpty())
            emit strategySelected(sid);
    }
}

void StrategyCatalogPanel::onFilterChanged(const QString& /*text*/)
{
    applyFilter();
}

void StrategyCatalogPanel::onStatusFilterChanged(int /*index*/)
{
    applyFilter();
}

} // namespace StrategyMgmt
