#ifndef BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H
#define BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H

// BacktestStrategySelector — left-panel strategy picker for the Backtest tab.
//
// Shows all live pipeline strategies from the Account→Portfolio→Strategy tree,
// grouped into a tree by Account / Portfolio.  Each strategy row shows its
// name, portfolio path, and the definition version badge (if a catalog binding
// is known).
//
// CPresenter populates the list via populate() after every backend change.
// When the user double-clicks (or presses Enter on) a strategy row the widget
// emits strategySelected() and CPresenter forwards it to the backtest panel.

#include <QWidget>
#include <QJsonObject>

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QPushButton;
class QLabel;

namespace BacktestUI {

struct StrategyListItem {
    QString     strategyId;        // live node UUID
    QString     strategyDefId;     // catalog def UUID (may be empty for legacy nodes)
    int         version    = 1;    // definition version
    QString     name;
    QString     accountName;
    QString     portfolioName;
    QJsonObject pipelineConfig;
};

class BacktestStrategySelector : public QWidget {
    Q_OBJECT
public:
    explicit BacktestStrategySelector(QWidget* parent = nullptr);

    // Replace the strategy list with a fresh snapshot from CPresenter.
    void populate(const QList<StrategyListItem>& items);

    // Programmatically select (highlight) the strategy row for strategyId.
    void highlightStrategy(const QString& strategyId);

signals:
    // Emitted on double-click or Enter; CPresenter routes this to the backtest panel.
    void strategySelected(const QString& strategyId,
                          const QString& displayName,
                          const QString& portfolioPath,
                          const QJsonObject& pipelineConfig);

    // Emitted when the user clicks the Refresh button.
    void refreshRequested();

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onFilterChanged(const QString& text);
    void onSelectClicked();

private:
    void buildUi();
    void applyFilter(const QString& text);

    QLineEdit*   m_searchEdit    = nullptr;
    QTreeWidget* m_tree          = nullptr;
    QPushButton* m_selectButton  = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QLabel*      m_countLabel    = nullptr;

    QList<StrategyListItem> m_items; // current full (unfiltered) list
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H
