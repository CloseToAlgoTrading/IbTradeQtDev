#include "BacktestUI/BacktestStrategySelector.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QFont>

namespace BacktestUI {

// ---------------------------------------------------------------------------
// Tree item roles
// ---------------------------------------------------------------------------
static constexpr int RoleStrategyId    = Qt::UserRole + 0;
static constexpr int RoleDefId         = Qt::UserRole + 1;
static constexpr int RoleVersion       = Qt::UserRole + 2;
static constexpr int RoleDisplayName   = Qt::UserRole + 3;
static constexpr int RolePortfolioPath = Qt::UserRole + 4;
static constexpr int RolePipelineJson  = Qt::UserRole + 5;
static constexpr int RoleIsStrategy    = Qt::UserRole + 6;

// ---------------------------------------------------------------------------
// BacktestStrategySelector
// ---------------------------------------------------------------------------

BacktestStrategySelector::BacktestStrategySelector(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void BacktestStrategySelector::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    // Header label
    auto* header = new QLabel(QStringLiteral("<b>Strategies</b>"), this);
    header->setStyleSheet(
        "font-size:12px; padding:4px 0; color:#c8d0da;");
    layout->addWidget(header);

    // Search box
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Filter strategies…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background:#2a2d36; border:1px solid #3a3d4a; "
        "border-radius:4px; padding:4px 8px; color:#c8d0da; } "
        "QLineEdit:focus { border-color:#4a90d9; }");
    layout->addWidget(m_searchEdit);

    // Tree
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({QStringLiteral("Strategy"), QStringLiteral("Ver")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tree->header()->setDefaultSectionSize(40);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setAnimated(true);
    m_tree->setStyleSheet(
        "QTreeWidget { background:#1e2029; border:1px solid #2d3040; "
        "border-radius:4px; color:#c8d0da; outline:none; } "
        "QTreeWidget::item { padding:4px 2px; } "
        "QTreeWidget::item:selected { background:#2a4a7a; } "
        "QTreeWidget::item:hover:!selected { background:#2a2d40; } "
        "QTreeWidget::branch:has-children:!has-siblings:closed,"
        "QTreeWidget::branch:closed:has-children:has-siblings { "
        "  border-image:none; image:url(:/icons/arrow-right.png); } "
        "QHeaderView::section { background:#252830; color:#888; "
        "border:none; border-bottom:1px solid #3a3d4a; padding:3px 6px; }");
    m_tree->setSortingEnabled(false);
    layout->addWidget(m_tree, 1);

    // Count label
    m_countLabel = new QLabel(QStringLiteral("0 strategies"), this);
    m_countLabel->setStyleSheet("color:#666; font-size:10px; padding:2px 0;");
    layout->addWidget(m_countLabel);

    // Buttons row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    m_selectButton = new QPushButton(QStringLiteral("Open in Backtest"), this);
    m_selectButton->setEnabled(false);
    m_selectButton->setStyleSheet(
        "QPushButton { background:#2a5298; color:#fff; border:none; "
        "border-radius:4px; padding:6px 12px; font-size:11px; } "
        "QPushButton:hover { background:#3a62a8; } "
        "QPushButton:disabled { background:#2a2d36; color:#555; }");

    m_refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    m_refreshButton->setStyleSheet(
        "QPushButton { background:#2a2d36; color:#c8d0da; border:1px solid #3a3d4a; "
        "border-radius:4px; padding:6px 12px; font-size:11px; } "
        "QPushButton:hover { background:#333645; }");

    btnRow->addWidget(m_selectButton, 1);
    btnRow->addWidget(m_refreshButton, 0);
    layout->addLayout(btnRow);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &BacktestStrategySelector::onFilterChanged);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this, &BacktestStrategySelector::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        auto sel = m_tree->selectedItems();
        bool hasStrategy = !sel.isEmpty() && sel.first()->data(0, RoleIsStrategy).toBool();
        m_selectButton->setEnabled(hasStrategy);
    });
    connect(m_selectButton, &QPushButton::clicked,
            this, &BacktestStrategySelector::onSelectClicked);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &BacktestStrategySelector::refreshRequested);

    setMinimumWidth(220);
    setMaximumWidth(360);
}

void BacktestStrategySelector::populate(const QList<StrategyListItem>& items)
{
    m_items = items;
    applyFilter(m_searchEdit ? m_searchEdit->text() : QString());
}

void BacktestStrategySelector::applyFilter(const QString& filter)
{
    if (!m_tree) return;
    m_tree->clear();

    const QString lf = filter.toLower().trimmed();

    // Group items by account → portfolio
    struct PortKey { QString account; QString portfolio; };
    QMap<QString, QTreeWidgetItem*> accountItems;
    QMap<QString, QTreeWidgetItem*> portfolioItems;

    int visibleCount = 0;

    for (const auto& item : m_items) {
        if (!lf.isEmpty() && !item.name.toLower().contains(lf)
                          && !item.portfolioName.toLower().contains(lf)
                          && !item.accountName.toLower().contains(lf))
            continue;

        ++visibleCount;

        // Account node
        const QString& acct = item.accountName;
        if (!accountItems.contains(acct)) {
            auto* ai = new QTreeWidgetItem(m_tree);
            ai->setText(0, acct);
            ai->setData(0, RoleIsStrategy, false);
            QFont f = ai->font(0);
            f.setBold(true);
            ai->setFont(0, f);
            ai->setForeground(0, QColor(QStringLiteral("#8aafd4")));
            ai->setFlags(ai->flags() & ~Qt::ItemIsSelectable);
            accountItems[acct] = ai;
        }
        QTreeWidgetItem* acctItem = accountItems[acct];

        // Portfolio node
        const QString portKey = acct + QLatin1Char('/') + item.portfolioName;
        if (!portfolioItems.contains(portKey)) {
            auto* pi = new QTreeWidgetItem(acctItem);
            pi->setText(0, item.portfolioName);
            pi->setData(0, RoleIsStrategy, false);
            pi->setForeground(0, QColor(QStringLiteral("#9faabf")));
            pi->setFlags(pi->flags() & ~Qt::ItemIsSelectable);
            portfolioItems[portKey] = pi;
        }
        QTreeWidgetItem* portItem = portfolioItems[portKey];

        // Strategy leaf
        auto* si = new QTreeWidgetItem(portItem);
        si->setText(0, item.name);
        si->setData(0, RoleStrategyId,    item.strategyId);
        si->setData(0, RoleDefId,         item.strategyDefId);
        si->setData(0, RoleVersion,       item.version);
        si->setData(0, RoleDisplayName,   item.name);
        si->setData(0, RolePortfolioPath,
                    item.accountName + QStringLiteral(" / ") + item.portfolioName);
        si->setData(0, RolePipelineJson,  item.pipelineConfig);
        si->setData(0, RoleIsStrategy,    true);
        si->setForeground(0, QColor(QStringLiteral("#e0e6f0")));

        // Version badge in col 1
        if (!item.strategyDefId.isEmpty()) {
            si->setText(1, QString("v%1").arg(item.version));
            si->setForeground(1, QColor(QStringLiteral("#4a90d9")));
        }
    }

    m_tree->expandAll();

    const QString label = visibleCount == 1
        ? QStringLiteral("1 strategy")
        : QString("%1 strategies").arg(visibleCount);
    m_countLabel->setText(label);
}

void BacktestStrategySelector::highlightStrategy(const QString& strategyId)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        if ((*it)->data(0, RoleStrategyId).toString() == strategyId) {
            m_tree->setCurrentItem(*it);
            m_tree->scrollToItem(*it);
            return;
        }
        ++it;
    }
}

void BacktestStrategySelector::onItemDoubleClicked(QTreeWidgetItem* item, int /*col*/)
{
    if (!item || !item->data(0, RoleIsStrategy).toBool()) return;

    const QString id     = item->data(0, RoleStrategyId).toString();
    const QString name   = item->data(0, RoleDisplayName).toString();
    const QString path   = item->data(0, RolePortfolioPath).toString();
    const QJsonObject cfg = item->data(0, RolePipelineJson).toJsonObject();

    emit strategySelected(id, name, path, cfg);
}

void BacktestStrategySelector::onFilterChanged(const QString& text)
{
    applyFilter(text);
}

void BacktestStrategySelector::onSelectClicked()
{
    auto sel = m_tree->selectedItems();
    if (sel.isEmpty()) return;
    onItemDoubleClicked(sel.first(), 0);
}

} // namespace BacktestUI
