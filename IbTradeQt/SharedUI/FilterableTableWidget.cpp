#include "SharedUI/FilterableTableWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QSet>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace {

constexpr int kCategoryRole = Qt::UserRole;

class TwoColumnFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit TwoColumnFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setTextFilter(const QString& t)
    {
        m_text = t;
        invalidateFilter();
    }

    void setCategoryFilter(const QString& canonical)
    {
        m_category = canonical;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QAbstractItemModel* m = sourceModel();
        if (!m)
            return false;

        const QModelIndex i0 = m->index(sourceRow, 0, sourceParent);
        const QModelIndex i1 = m->index(sourceRow, 1, sourceParent);
        if (!i0.isValid() || !i1.isValid())
            return false;

        const QString cat = m->data(i1, kCategoryRole).toString();
        if (!m_category.isEmpty() && cat != m_category)
            return false;

        const QString t0 = m->data(i0, Qt::DisplayRole).toString();
        const QString t1 = m->data(i1, Qt::DisplayRole).toString();
        const QString needle = m_text.trimmed();
        if (needle.isEmpty())
            return true;
        return t0.contains(needle, Qt::CaseInsensitive)
            || t1.contains(needle, Qt::CaseInsensitive);
    }

private:
    QString m_text;
    QString m_category;
};

} // namespace

FilterableTableWidget::FilterableTableWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("FilterableTableWidget"));

    auto* model = new QStandardItemModel(this);
    model->setColumnCount(2);

    auto* proxy = new TwoColumnFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setObjectName(QStringLiteral("filterableTableFilterEdit"));
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setPlaceholderText(QStringLiteral("Filter by ticker or text…"));

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setObjectName(QStringLiteral("filterableTableStatusCombo"));

    m_table = new QTableView(this);
    m_table->setObjectName(QStringLiteral("filterableTableView"));
    m_table->setModel(proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setShowGrid(true);
    m_table->setWordWrap(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setMinimumHeight(160);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [proxy](const QString& t) {
        proxy->setTextFilter(t);
    });
    connect(m_statusCombo, &QComboBox::currentIndexChanged, this, [this, proxy]() {
        const QString cat = m_statusCombo->currentData().toString();
        proxy->setCategoryFilter(cat);
    });

    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(8);
    auto* fl = new QLabel(QStringLiteral("Filter"), this);
    fl->setObjectName(QStringLiteral("filterableTableFilterLabel"));
    auto* sl = new QLabel(QStringLiteral("Status"), this);
    sl->setObjectName(QStringLiteral("filterableTableStatusLabel"));
    filterRow->addWidget(fl);
    filterRow->addWidget(m_filterEdit, 1);
    filterRow->addWidget(sl);
    filterRow->addWidget(m_statusCombo);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(8);
    lay->addLayout(filterRow);
    lay->addWidget(m_table, 1);

    setColumnHeaders(QStringLiteral("Ticker"), QStringLiteral("Status"));
}

void FilterableTableWidget::setColumnHeaders(const QString& col0Label, const QString& col1Label)
{
    auto* m = qobject_cast<QStandardItemModel*>(
        qobject_cast<QSortFilterProxyModel*>(m_table->model())->sourceModel());
    if (!m)
        return;
    m->setHorizontalHeaderLabels({col0Label, col1Label});
}

void FilterableTableWidget::setFilterPlaceholder(const QString& text)
{
    m_filterEdit->setPlaceholderText(text);
}

void FilterableTableWidget::rebuildStatusFilterOptions(const QVector<Row>& rows)
{
    QSet<QString> present;
    for (const Row& r : rows) {
        if (!r.statusCategory.isEmpty())
            present.insert(r.statusCategory);
    }

    const struct {
        QString canon;
        QString label;
    } kAll[] = {
        {QStringLiteral("fully_cached"), QStringLiteral("Fully cached")},
        {QStringLiteral("needs_fetch"), QStringLiteral("Needs download")},
        {QStringLiteral("partial_gap"), QStringLiteral("Partial gap")},
        {QStringLiteral("yahoo_error"), QStringLiteral("Yahoo validation")},
    };

    const QSignalBlocker blocker(m_statusCombo);
    m_statusCombo->clear();
    m_statusCombo->addItem(QStringLiteral("All"), QString());

    for (const auto& e : kAll) {
        if (present.contains(e.canon))
            m_statusCombo->addItem(e.label, e.canon);
    }
}

void FilterableTableWidget::setRows(const QVector<Row>& rows)
{
    rebuildStatusFilterOptions(rows);

    auto* proxy = qobject_cast<QSortFilterProxyModel*>(m_table->model());
    auto* m     = qobject_cast<QStandardItemModel*>(proxy ? proxy->sourceModel() : nullptr);
    if (!m)
        return;

    m->removeRows(0, m->rowCount());

    for (const Row& r : rows) {
        auto* i0 = new QStandardItem(r.col0);
        auto* i1 = new QStandardItem(r.col1);
        i1->setData(r.statusCategory, kCategoryRole);
        i0->setEditable(false);
        i1->setEditable(false);
        m->appendRow({i0, i1});
    }

    m_table->sortByColumn(0, Qt::AscendingOrder);
}

void FilterableTableWidget::clear()
{
    setRows({});
}
