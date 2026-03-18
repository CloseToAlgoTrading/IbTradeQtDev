#ifndef STRATEGYMANAGEMENTPANEL_H
#define STRATEGYMANAGEMENTPANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

namespace StrategyMgmt {

class StrategyCatalogPanel;
class StrategyDetailPanel;

// Main widget for the "Strategy Management" tab.
// Combines StrategyCatalogPanel (left) + StrategyDetailPanel (right) in a splitter.
class StrategyManagementPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyManagementPanel(QWidget* parent = nullptr);

    StrategyCatalogPanel* catalogPanel() const { return m_catalogPanel; }
    StrategyDetailPanel*  detailPanel()  const { return m_detailPanel; }

    // Populate catalog from backend data.
    void populateCatalog(const QJsonArray& catalogEntries,
                         const QMap<QString, int>& versionCounts);

    // Show details for a specific strategy.
    void showStrategyDetail(const QJsonObject& catalogEntry,
                            const QJsonArray& versions);

signals:
    // Re-emitted from child panels for the presenter to handle
    void strategySelected(const QString& strategyId);
    void newStrategyRequested();
    void metadataChanged(const QString& strategyId, const QString& name,
                         const QString& description, const QString& tags,
                         const QString& lifecycleState);
    void newVersionRequested(const QString& strategyId);
    void publishRequested(const QString& strategyId, const QString& versionId);
    void archiveRequested(const QString& strategyId);
    void useInLiveRequested(const QString& strategyId, const QString& versionId);
    void openInBacktestRequested(const QString& strategyId, const QString& versionId);

private:
    void buildUi();

    StrategyCatalogPanel* m_catalogPanel = nullptr;
    StrategyDetailPanel*  m_detailPanel  = nullptr;
};

} // namespace StrategyMgmt

#endif // STRATEGYMANAGEMENTPANEL_H
