#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyTreePanel.h"
#include "PipelineConstants.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QJsonDocument>

namespace BacktestUI {

BacktestStrategySelector::BacktestStrategySelector(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void BacktestStrategySelector::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    auto* header = new QLabel(QStringLiteral("<b>Strategies</b>"), this);
    header->setObjectName(QStringLiteral("panelHeader"));
    layout->addWidget(header);

    // Tree panel (reusable component)
    m_treePanel = new StrategyTreePanel(this);
    m_treePanel->setSearchVisible(true);
    m_treePanel->searchEdit()->setPlaceholderText(
        QStringLiteral("Filter strategies\u2026"));

    m_model = new BacktestTreeModel(this);
    m_treePanel->setModel(m_model);

    // Column sizing
    auto* tv = m_treePanel->treeView();
    tv->header()->setSectionResizeMode(BacktestTreeModel::ColName, QHeaderView::Stretch);
    tv->header()->setSectionResizeMode(BacktestTreeModel::ColVersion, QHeaderView::Fixed);
    tv->header()->setDefaultSectionSize(50);

    layout->addWidget(m_treePanel, 1);

    // Count label
    m_countLabel = new QLabel(QStringLiteral("0 strategies"), this);
    m_countLabel->setObjectName(QStringLiteral("countLabel"));
    layout->addWidget(m_countLabel);

    // Buttons row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);
    btnRow->setContentsMargins(4, 2, 4, 4);

    m_selectButton = new QPushButton(QStringLiteral("Open in Backtest"), this);
    m_selectButton->setEnabled(false);
    m_refreshButton = new QPushButton(QStringLiteral("Refresh"), this);

    btnRow->addWidget(m_selectButton, 1);
    btnRow->addWidget(m_refreshButton, 0);
    layout->addLayout(btnRow);

    // Connections
    connect(tv, &QTreeView::doubleClicked,
            this, &BacktestStrategySelector::onItemDoubleClicked);
    connect(tv->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex& current, const QModelIndex&) {
        QModelIndex src = mapToSource(current);
        bool isSel = src.isValid()
                     && src.data(BacktestTreeModel::IsStrategyRole).toBool();
        m_selectButton->setEnabled(isSel);
    });
    connect(m_selectButton, &QPushButton::clicked,
            this, &BacktestStrategySelector::onSelectClicked);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &BacktestStrategySelector::refreshRequested);

    setMinimumWidth(220);
    setMaximumWidth(360);
}

QModelIndex BacktestStrategySelector::mapToSource(const QModelIndex& proxyIndex) const
{
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(
        m_treePanel->treeView()->model());
    return proxy ? proxy->mapToSource(proxyIndex) : proxyIndex;
}

void BacktestStrategySelector::populate(const QList<StrategyListItem>& items)
{
    m_model->populate(items);
    m_treePanel->expandAll();

    int count = items.size();
    m_countLabel->setText(count == 1 ? QStringLiteral("1 strategy")
                                     : QString("%1 strategies").arg(count));
}

void BacktestStrategySelector::populateCatalog(const QList<CatalogVersionItem>& catalogItems)
{
    m_model->populateCatalog(catalogItems);
    m_treePanel->expandAll();
}

void BacktestStrategySelector::highlightStrategy(const QString& strategyId)
{
    // Walk all model indices to find the matching strategy
    std::function<QModelIndex(const QModelIndex&)> findIdx;
    findIdx = [&](const QModelIndex& parent) -> QModelIndex {
        int rows = m_model->rowCount(parent);
        for (int r = 0; r < rows; ++r) {
            QModelIndex idx = m_model->index(r, 0, parent);
            if (idx.data(BacktestTreeModel::StrategyIdRole).toString() == strategyId)
                return idx;
            QModelIndex child = findIdx(idx);
            if (child.isValid()) return child;
        }
        return {};
    };

    QModelIndex src = findIdx({});
    if (!src.isValid()) return;

    auto* proxy = qobject_cast<QSortFilterProxyModel*>(
        m_treePanel->treeView()->model());
    QModelIndex mapped = proxy ? proxy->mapFromSource(src) : src;
    m_treePanel->treeView()->setCurrentIndex(mapped);
    m_treePanel->treeView()->scrollTo(mapped);
}

void BacktestStrategySelector::onItemDoubleClicked(const QModelIndex& proxyIndex)
{
    QModelIndex index = mapToSource(proxyIndex);
    if (!index.isValid()) return;

    // Block leaf?
    if (m_model->isVirtualBlock(index)) {
        QString category = m_model->virtualCategory(index);
        QString blockId  = m_model->virtualBlockId(index);

        // Walk up to strategy ancestor
        QModelIndex ancestor = index.parent();
        while (ancestor.isValid() && m_model->isVirtualCategory(ancestor))
            ancestor = ancestor.parent();

        // Find pipeline config from the ancestor
        QJsonObject pipeCfg = ancestor.data(BacktestTreeModel::PipelineJsonRole).toJsonObject();

        bool isArray = Pipeline::categoryIsArray(category);
        QString jsonKey = QString(Pipeline::categoryKey(category));

        int arrayIndex = 0;
        if (isArray) {
            QModelIndex catIdx = index.parent();
            arrayIndex = index.row();
        }

        emit blockSelected(category, jsonKey, isArray, arrayIndex, pipeCfg);
        return;
    }

    if (!index.data(BacktestTreeModel::IsStrategyRole).toBool())
        return;

    if (index.data(BacktestTreeModel::IsCatalogEntryRole).toBool()) {
        emit catalogVersionSelected(
            index.data(BacktestTreeModel::CatalogStratIdRole).toString(),
            index.data(BacktestTreeModel::CatalogVerIdRole).toString());
        return;
    }

    emit strategySelected(
        index.data(BacktestTreeModel::StrategyIdRole).toString(),
        index.data(BacktestTreeModel::DisplayNameRole).toString(),
        index.data(BacktestTreeModel::PortfolioPathRole).toString(),
        index.data(BacktestTreeModel::PipelineJsonRole).toJsonObject());
}

void BacktestStrategySelector::onSelectClicked()
{
    QModelIndex current = m_treePanel->treeView()->currentIndex();
    if (current.isValid())
        onItemDoubleClicked(current);
}

} // namespace BacktestUI
