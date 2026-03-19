#ifndef STRATEGYTREEPANEL_H
#define STRATEGYTREEPANEL_H

#include <QWidget>

class QTreeView;
class QLineEdit;
class QVBoxLayout;
class QSortFilterProxyModel;
class QAbstractItemModel;
class StrategyTreeDelegate;

// Reusable panel widget used by Live Trading, Backtest, and Strategy
// Management views.  Wraps a QTreeView with the shared delegate,
// an optional search bar, and an optional toolbar area.
//
// All visual styling comes from operations-console.qss — no inline
// stylesheets are applied.

class StrategyTreePanel : public QWidget
{
    Q_OBJECT
public:
    explicit StrategyTreePanel(QWidget* parent = nullptr);

    // Model
    void setModel(QAbstractItemModel* model);
    QAbstractItemModel* model() const;

    // Search bar (hidden by default)
    void setSearchVisible(bool visible);
    bool isSearchVisible() const;
    QLineEdit* searchEdit() const { return m_searchEdit; }

    // Toolbar: caller owns the widget, panel just inserts it above the tree
    void setToolbarWidget(QWidget* toolbar);

    // Direct access to the underlying tree
    QTreeView* treeView() const { return m_tree; }

    // Convenience
    void expandAll();
    void collapseAll();

signals:
    void searchTextChanged(const QString& text);

private:
    void buildUi();

    QVBoxLayout*           m_layout       = nullptr;
    QLineEdit*             m_searchEdit   = nullptr;
    QWidget*               m_toolbar      = nullptr;
    QTreeView*             m_tree         = nullptr;
    StrategyTreeDelegate*  m_delegate     = nullptr;
    QSortFilterProxyModel* m_filterProxy  = nullptr;
    QAbstractItemModel*    m_sourceModel  = nullptr;
};

#endif // STRATEGYTREEPANEL_H
