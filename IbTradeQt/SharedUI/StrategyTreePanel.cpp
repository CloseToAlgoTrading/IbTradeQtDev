#include "StrategyTreePanel.h"
#include "StrategyTreeDelegate.h"

#include <QTreeView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSortFilterProxyModel>

StrategyTreePanel::StrategyTreePanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyTreePanel::buildUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Search bar (hidden by default)
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setVisible(false);
    m_layout->addWidget(m_searchEdit);

    // Tree view
    m_tree = new QTreeView(this);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setAnimated(true);
    m_tree->setHeaderHidden(false);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tree->header()->setMinimumSectionSize(40);

    m_delegate = new StrategyTreeDelegate(this);
    m_tree->setItemDelegate(m_delegate);

    m_layout->addWidget(m_tree, 1);

    // Filter proxy (used only when search is enabled)
    m_filterProxy = new QSortFilterProxyModel(this);
    m_filterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterProxy->setRecursiveFilteringEnabled(true);
    m_filterProxy->setFilterKeyColumn(0);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_filterProxy->setFilterFixedString(text);
        if (!text.isEmpty())
            m_tree->expandAll();
        emit searchTextChanged(text);
    });
}

void StrategyTreePanel::setModel(QAbstractItemModel* model)
{
    m_sourceModel = model;
    m_filterProxy->setSourceModel(model);
    m_tree->setModel(m_filterProxy);

    // First n-1 columns: user-resizable; last column stretches to fill remaining
    // viewport width (no empty gap). If leading columns' minimum widths exceed the
    // viewport, horizontal scroll still appears.
    if (model && model->columnCount() > 0) {
        m_tree->header()->setStretchLastSection(true);
        const int n = model->columnCount();
        for (int i = 0; i < n - 1; ++i)
            m_tree->header()->setSectionResizeMode(i, QHeaderView::Interactive);
        for (int i = 0; i < n - 1; ++i)
            m_tree->resizeColumnToContents(i);
    }
}

QAbstractItemModel* StrategyTreePanel::model() const
{
    return m_sourceModel;
}

void StrategyTreePanel::setSearchVisible(bool visible)
{
    m_searchEdit->setVisible(visible);
    if (!visible) {
        m_searchEdit->clear();
        m_filterProxy->setFilterFixedString({});
    }
}

bool StrategyTreePanel::isSearchVisible() const
{
    return m_searchEdit->isVisible();
}

void StrategyTreePanel::setToolbarWidget(QWidget* toolbar)
{
    if (m_toolbar) {
        m_layout->removeWidget(m_toolbar);
        m_toolbar->setParent(nullptr);
    }
    m_toolbar = toolbar;
    if (toolbar) {
        toolbar->setParent(this);
        // Insert before the tree (index 0 if no search, 1 if search visible)
        int insertIdx = m_searchEdit->isVisible() ? 1 : 0;
        m_layout->insertWidget(insertIdx, toolbar);
    }
}

void StrategyTreePanel::expandAll()
{
    m_tree->expandAll();
}

void StrategyTreePanel::collapseAll()
{
    m_tree->collapseAll();
}
