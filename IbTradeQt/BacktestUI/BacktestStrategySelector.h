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
    QString     strategyId;        // live node UUID (empty for catalog-only entries)
    QString     strategyDefId;     // catalog strategy UUID (may be empty for legacy nodes)
    QString     catalogVersionId;  // catalog version UUID
    int         version    = 1;    // definition version
    QString     name;
    QString     accountName;
    QString     portfolioName;
    QJsonObject pipelineConfig;
};

struct CatalogVersionItem {
    QString strategyId;
    QString strategyName;
    QString versionId;
    int     versionNumber = 1;
    QString configJson;
    bool    isPublished   = false;
};

class BacktestStrategySelector : public QWidget {
    Q_OBJECT
public:
    explicit BacktestStrategySelector(QWidget* parent = nullptr);

    void populate(const QList<StrategyListItem>& items);
    void populateCatalog(const QList<CatalogVersionItem>& catalogItems);

    void highlightStrategy(const QString& strategyId);

signals:
    void strategySelected(const QString& strategyId,
                          const QString& displayName,
                          const QString& portfolioPath,
                          const QJsonObject& pipelineConfig);

    void catalogVersionSelected(const QString& catalogStrategyId,
                                const QString& catalogVersionId);

    void blockSelected(const QString& category, const QString& jsonKey,
                       bool isArray, int arrayIndex,
                       const QJsonObject& pipelineConfig);

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

    QList<StrategyListItem>   m_items;
    QList<CatalogVersionItem> m_catalogItems;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H
