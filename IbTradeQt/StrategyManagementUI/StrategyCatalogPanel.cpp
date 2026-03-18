#include "StrategyCatalogPanel.h"
#include "StrategyCatalogModel.h"

#include <QTreeView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace StrategyMgmt {

class CatalogFilterProxy : public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setSearchText(const QString& text)  { m_search = text.toLower(); invalidateFilter(); }
    void setStatusFilter(const QString& st)  { m_status = st; invalidateFilter(); }

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override {
        auto idx = [&](int col) { return sourceModel()->index(row, col, parent); };

        if (!m_status.isEmpty()) {
            QString rowStatus = idx(StrategyCatalogModel::ColStatus).data().toString();
            if (rowStatus != m_status) return false;
        }
        if (!m_search.isEmpty()) {
            QString name = idx(StrategyCatalogModel::ColName).data().toString().toLower();
            if (!name.contains(m_search)) return false;
        }
        return true;
    }

private:
    QString m_search;
    QString m_status;
};

StrategyCatalogPanel::StrategyCatalogPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyCatalogPanel::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Top bar: search + status filter + New button
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

    // Model + proxy
    m_model = new StrategyCatalogModel(this);
    m_proxyModel = new CatalogFilterProxy(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    // Tree view
    m_treeView = new QTreeView;
    m_treeView->setModel(m_proxyModel);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(StrategyCatalogModel::ColLastUpdated, Qt::DescendingOrder);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_treeView, 1);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &StrategyCatalogPanel::onFilterChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StrategyCatalogPanel::onStatusFilterChanged);
    connect(m_newButton, &QPushButton::clicked,
            this, &StrategyCatalogPanel::newStrategyRequested);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this]{ onSelectionChanged(); });
}

void StrategyCatalogPanel::populate(const QJsonArray& catalogEntries,
                                     const QMap<QString, int>& versionCounts)
{
    m_model->resetData(catalogEntries, versionCounts);

    // Reconnect selection signal (model reset clears selection model)
    disconnect(m_treeView->selectionModel(), nullptr, this, nullptr);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this]{ onSelectionChanged(); });
}

void StrategyCatalogPanel::onSelectionChanged()
{
    QModelIndex proxyIdx = m_treeView->currentIndex();
    if (!proxyIdx.isValid()) return;

    QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
    QString strategyId = m_model->data(srcIdx, StrategyCatalogModel::StrategyIdRole).toString();
    if (!strategyId.isEmpty())
        emit strategySelected(strategyId);
}

void StrategyCatalogPanel::onFilterChanged(const QString& text)
{
    static_cast<CatalogFilterProxy*>(m_proxyModel)->setSearchText(text);
}

void StrategyCatalogPanel::onStatusFilterChanged(int index)
{
    QString status = (index == 0) ? QString() : m_statusCombo->currentText();
    static_cast<CatalogFilterProxy*>(m_proxyModel)->setStatusFilter(status);
}

} // namespace StrategyMgmt
