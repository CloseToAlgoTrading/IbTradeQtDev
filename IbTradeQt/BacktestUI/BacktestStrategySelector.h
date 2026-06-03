#ifndef BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H
#define BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H

#include <QWidget>
#include <QJsonObject>
#include "BacktestTreeModel.h"

class StrategyTreePanel;
class QTreeView;
class QPushButton;
class QLabel;

namespace BacktestUI {

class BacktestStrategySelector : public QWidget {
    Q_OBJECT
public:
    explicit BacktestStrategySelector(QWidget* parent = nullptr);

    QTreeView* strategyTreeView() const;

    void populate(const QList<StrategyListItem>& items);
    void populateCatalog(const QList<CatalogVersionItem>& catalogItems);
    void highlightStrategy(const QString& strategyId);
    QJsonObject currentPipelineConfig() const;

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

    void pipelineContextSelected(const QJsonObject& pipelineConfig);

    void refreshRequested();

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onSelectClicked();

private:
    void buildUi();
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
    QJsonObject pipelineConfigForIndex(const QModelIndex& sourceIndex) const;

    StrategyTreePanel* m_treePanel     = nullptr;
    BacktestTreeModel* m_model         = nullptr;
    QPushButton*       m_selectButton  = nullptr;
    QPushButton*       m_refreshButton = nullptr;
    QLabel*            m_countLabel    = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTSTRATEGYSELECTOR_H
