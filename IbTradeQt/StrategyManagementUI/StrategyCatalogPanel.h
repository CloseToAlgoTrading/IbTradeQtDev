#ifndef STRATEGYCATALOGPANEL_H
#define STRATEGYCATALOGPANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace StrategyMgmt {

class StrategyCatalogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyCatalogPanel(QWidget* parent = nullptr);

    void populate(const QJsonArray& catalogEntries,
                  const QMap<QString, int>& versionCounts,
                  const QMap<QString, QJsonObject>& latestConfigs);

signals:
    void strategySelected(const QString& strategyId);
    void blockSelected(const QString& strategyId,
                       const QString& category, const QString& jsonKey,
                       bool isArray, int arrayIndex);
    void newStrategyRequested();

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onFilterChanged(const QString& text);
    void onStatusFilterChanged(int index);

private:
    void buildUi();
    void applyFilter();

    enum ItemRole {
        RoleStrategyId  = Qt::UserRole + 50,
        RoleIsStrategy  = Qt::UserRole + 51,
        RoleStatus      = Qt::UserRole + 52,
    };

    struct CatalogEntry {
        QString     strategyId;
        QString     name;
        int         strategyKind = 0;
        QString     lifecycleState;
        int         versionCount = 0;
        QString     updatedAt;
        QJsonObject pipelineConfig;
    };

    QList<CatalogEntry> m_entries;

    QTreeWidget* m_tree        = nullptr;
    QLineEdit*   m_searchEdit  = nullptr;
    QComboBox*   m_statusCombo = nullptr;
    QPushButton* m_newButton   = nullptr;
};

} // namespace StrategyMgmt

#endif // STRATEGYCATALOGPANEL_H
