#include "StrategyCatalogPanel.h"
#include "CatalogTreeModel.h"
#include "StrategyTreePanel.h"
#include "PipelineConstants.h"
#include "Pipeline/BlockRegistry.h"

#include <QTreeView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QSortFilterProxyModel>

namespace StrategyMgmt {

StrategyCatalogPanel::StrategyCatalogPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyCatalogPanel::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Toolbar
    auto* toolbar = new QWidget;
    auto* topBar = new QHBoxLayout(toolbar);
    topBar->setContentsMargins(4, 4, 4, 4);
    topBar->setSpacing(4);

    m_statusCombo = new QComboBox;
    m_statusCombo->addItem(QStringLiteral("All"));
    m_statusCombo->addItem(QStringLiteral("draft"));
    m_statusCombo->addItem(QStringLiteral("active"));
    m_statusCombo->addItem(QStringLiteral("archived"));
    topBar->addWidget(m_statusCombo);

    m_newButton = new QPushButton(QStringLiteral("New Strategy"));
    topBar->addWidget(m_newButton);

    // Tree panel (shared component)
    m_treePanel = new StrategyTreePanel(this);
    m_treePanel->setSearchVisible(true);
    m_treePanel->searchEdit()->setPlaceholderText(
        QStringLiteral("Search strategies..."));
    m_treePanel->setToolbarWidget(toolbar);

    m_model = new CatalogTreeModel(this);
    m_treePanel->setModel(m_model);

    auto* tv = m_treePanel->treeView();
    tv->header()->setStretchLastSection(true);
    tv->setContextMenuPolicy(Qt::CustomContextMenu);

    layout->addWidget(m_treePanel, 1);

    // Connections
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StrategyCatalogPanel::onStatusFilterChanged);
    connect(m_newButton, &QPushButton::clicked,
            this, &StrategyCatalogPanel::newStrategyRequested);
    connect(tv, &QTreeView::clicked,
            this, &StrategyCatalogPanel::onItemClicked);
    connect(tv, &QTreeView::customContextMenuRequested,
            this, &StrategyCatalogPanel::onContextMenu);
}

QModelIndex StrategyCatalogPanel::mapToSource(const QModelIndex& proxyIndex) const
{
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(
        m_treePanel->treeView()->model());
    return proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
}

void StrategyCatalogPanel::populate(const QJsonArray& catalogEntries,
                                     const QMap<QString, int>& versionCounts,
                                     const QMap<QString, QJsonObject>& latestConfigs)
{
    m_model->populate(catalogEntries, versionCounts, latestConfigs);
    m_treePanel->expandAll();
}

void StrategyCatalogPanel::onItemClicked(const QModelIndex& proxyIndex)
{
    QModelIndex index = mapToSource(proxyIndex);
    if (!index.isValid()) return;

    // Block leaf?
    if (m_model->isBlockLeaf(index)) {
        QString strategyId = m_model->strategyIdFor(index);
        emit blockSelected(strategyId,
                           m_model->blockCategory(index),
                           m_model->blockJsonKey(index),
                           m_model->blockIsArray(index),
                           m_model->blockArrayIndex(index));
        return;
    }

    // Strategy row?
    if (index.data(CatalogTreeModel::IsStrategyRole).toBool()) {
        QString sid = index.data(CatalogTreeModel::StrategyIdRole).toString();
        if (!sid.isEmpty())
            emit strategySelected(sid);
    }
}

void StrategyCatalogPanel::onStatusFilterChanged(int /*index*/)
{
    // Re-populate with current data to apply filter
    // The model itself handles all data; we just need to re-filter.
    // For now, repopulate — a future improvement could use proxy filtering.
    m_treePanel->expandAll();
}

void StrategyCatalogPanel::onContextMenu(const QPoint& pos)
{
    auto* tv = m_treePanel->treeView();
    QModelIndex proxyIdx = tv->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QModelIndex index = mapToSource(proxyIdx);
    if (!index.isValid()) return;

    QMenu menu;
    QMap<QString, QAction*> addBlockActions;

    bool isBlock    = m_model->isBlockLeaf(index);
    bool isStrategy = index.data(CatalogTreeModel::IsStrategyRole).toBool();
    bool isCategory = m_model->isVirtualCategory(index);

    // Strategy-level or category-level: offer block addition
    if (isStrategy || isCategory) {
        static const QVector<QPair<QString, QString>> blockCategories = {
            { Pipeline::Category::Selection, QStringLiteral("Add Selection Model") },
            { Pipeline::Category::Alpha,     QStringLiteral("Add Alpha Model")     },
            { Pipeline::Category::Rebalance, QStringLiteral("Add Rebalance Model") },
            { Pipeline::Category::Risk,      QStringLiteral("Add Risk Model")      },
            { Pipeline::Category::Execution, QStringLiteral("Add Execution Model") },
        };

        auto& registry = Pipeline::BlockRegistry::instance();
        for (const auto& [category, label] : blockCategories) {
            auto blocks = registry.blocksByCategory(category);
            if (blocks.isEmpty()) {
                QAction* act = menu.addAction(label + "...");
                act->setEnabled(false);
            } else if (blocks.size() == 1) {
                QAction* act = menu.addAction(label + ": " + blocks.first().name);
                addBlockActions[category + "|" + blocks.first().id] = act;
            } else {
                QMenu* sub = menu.addMenu(label);
                for (const auto& desc : blocks) {
                    QAction* act = sub->addAction(desc.name);
                    if (!desc.description.isEmpty())
                        act->setToolTip(desc.description);
                    addBlockActions[category + "|" + desc.id] = act;
                }
            }
        }
    }

    // Block-level actions
    QAction* removeAction = nullptr;
    QAction* editAction = nullptr;
    if (isBlock) {
        editAction   = menu.addAction(QStringLiteral("Edit Parameters"));
        menu.addSeparator();
        removeAction = menu.addAction(QStringLiteral("Remove Block"));
    }

    if (menu.isEmpty()) return;

    QAction* chosen = menu.exec(tv->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    QString sid = m_model->strategyIdFor(index);
    if (sid.isEmpty()) return;

    if (chosen == editAction && isBlock) {
        emit blockSelected(sid,
                           m_model->blockCategory(index),
                           m_model->blockJsonKey(index),
                           m_model->blockIsArray(index),
                           m_model->blockArrayIndex(index));
        return;
    }

    if (chosen == removeAction && isBlock) {
        emit removeBlockRequested(sid,
                                  m_model->blockCategory(index),
                                  m_model->blockArrayIndex(index));
        return;
    }

    for (auto it = addBlockActions.constBegin(); it != addBlockActions.constEnd(); ++it) {
        if (it.value() == chosen) {
            QStringList parts = it.key().split('|');
            if (parts.size() == 2) {
                auto& registry = Pipeline::BlockRegistry::instance();
                auto desc = registry.descriptor(parts[1]);
                QJsonObject defaultCfg = desc ? desc->defaultConfig : QJsonObject{};
                emit addBlockRequested(sid, parts[0], parts[1], defaultCfg);
            }
            break;
        }
    }
}

} // namespace StrategyMgmt
