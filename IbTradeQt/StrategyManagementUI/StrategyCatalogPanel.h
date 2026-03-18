#ifndef STRATEGYCATALOGPANEL_H
#define STRATEGYCATALOGPANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QMap>

class QTreeView;
class QLineEdit;
class QComboBox;
class QPushButton;
class QSortFilterProxyModel;

namespace StrategyMgmt {

class StrategyCatalogModel;

// Left panel of the Strategy Management tab: search, filter, and flat list
// of strategy families with columns: Name, Kind, Status, Versions, Last Updated.
class StrategyCatalogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyCatalogPanel(QWidget* parent = nullptr);

    // Populate from backend data. versionCounts maps strategyId → version count.
    void populate(const QJsonArray& catalogEntries,
                  const QMap<QString, int>& versionCounts);

    StrategyCatalogModel* model() const { return m_model; }

signals:
    void strategySelected(const QString& strategyId);
    void newStrategyRequested();

private slots:
    void onSelectionChanged();
    void onFilterChanged(const QString& text);
    void onStatusFilterChanged(int index);

private:
    void buildUi();

    StrategyCatalogModel*    m_model       = nullptr;
    QSortFilterProxyModel*   m_proxyModel  = nullptr;
    QTreeView*               m_treeView    = nullptr;
    QLineEdit*               m_searchEdit  = nullptr;
    QComboBox*               m_statusCombo = nullptr;
    QPushButton*             m_newButton   = nullptr;
};

} // namespace StrategyMgmt

#endif // STRATEGYCATALOGPANEL_H
