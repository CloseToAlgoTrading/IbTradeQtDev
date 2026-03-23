#ifndef STRATEGYCATALOGPANEL_H
#define STRATEGYCATALOGPANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

class StrategyTreePanel;
class QComboBox;
class QPushButton;

namespace StrategyMgmt {

class CatalogTreeModel;

class StrategyCatalogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyCatalogPanel(QWidget* parent = nullptr);

    StrategyTreePanel* treePanel() const { return m_treePanel; }

    void populate(const QJsonArray& catalogEntries,
                  const QMap<QString, int>& versionCounts,
                  const QMap<QString, QJsonObject>& latestConfigs);

signals:
    void strategySelected(const QString& strategyId);
    void blockSelected(const QString& strategyId,
                       const QString& category, const QString& jsonKey,
                       bool isArray, int arrayIndex);
    void newStrategyRequested();
    void addBlockRequested(const QString& strategyId,
                           const QString& category,
                           const QString& blockId,
                           const QJsonObject& defaultConfig);
    void removeBlockRequested(const QString& strategyId,
                              const QString& category,
                              int blockIndex);
    void deleteStrategyRequested(const QString& strategyId);

private slots:
    void onItemClicked(const QModelIndex& proxyIndex);
    void onStatusFilterChanged(int index);
    void onContextMenu(const QPoint& pos);

private:
    void buildUi();
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const;

    StrategyTreePanel* m_treePanel   = nullptr;
    CatalogTreeModel*  m_model       = nullptr;
    QComboBox*         m_statusCombo = nullptr;
    QPushButton*       m_newButton   = nullptr;

    // Status filter — applied via the search proxy
    QString m_currentStatusFilter;
};

} // namespace StrategyMgmt

#endif // STRATEGYCATALOGPANEL_H
