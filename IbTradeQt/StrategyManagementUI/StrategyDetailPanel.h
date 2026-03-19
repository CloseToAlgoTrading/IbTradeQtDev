#ifndef STRATEGYDETAILPANEL_H
#define STRATEGYDETAILPANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>

class QLabel;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;
class QTabWidget;
class BlockInspectorPanel;

namespace StrategyMgmt {

class StrategyDetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyDetailPanel(QWidget* parent = nullptr);

    void showStrategy(const QJsonObject& catalogEntry, const QJsonArray& versions);
    void clear();

    void showBlockDetails(const QString& category, const QString& jsonKey,
                          bool isArray, int arrayIndex);
    void hideBlockDetails();

    QJsonObject workingConfig() const { return m_workingConfig; }
    bool isConfigDirty() const { return m_configDirty; }

signals:
    void metadataChanged(const QString& strategyId,
                         const QString& name,
                         const QString& description,
                         const QString& tags,
                         const QString& lifecycleState);

    void newVersionRequested(const QString& strategyId,
                             const QJsonObject& pipelineConfig);
    void publishRequested(const QString& strategyId, const QString& versionId);
    void archiveRequested(const QString& strategyId);
    void useInLiveRequested(const QString& strategyId, const QString& versionId);
    void openInBacktestRequested(const QString& strategyId, const QString& versionId);

    void addBlockRequested(const QString& strategyId,
                           const QString& category,
                           const QString& blockId,
                           const QJsonObject& defaultConfig);
    void removeBlockRequested(const QString& strategyId,
                              const QString& category,
                              int blockIndex);

private slots:
    void onVersionSelected(int row, int column);
    void onSaveMetadata();
    void onNewVersion();
    void onPublish();
    void onArchive();
    void onUseInLive();
    void onOpenInBacktest();

private:
    void buildUi();
    QString selectedVersionId() const;
    void markDirty();

    // Metadata
    QLineEdit*      m_nameEdit      = nullptr;
    QLabel*         m_kindLabel     = nullptr;
    QComboBox*      m_statusCombo   = nullptr;
    QTextEdit*      m_descEdit      = nullptr;
    QLineEdit*      m_tagsEdit      = nullptr;
    QPushButton*    m_saveMetaBtn   = nullptr;

    // Version table
    QTableWidget*   m_versionTable  = nullptr;

    // Block inspector (dynamic tab, shown on block selection)
    BlockInspectorPanel*  m_inspector     = nullptr;
    int                   m_inspectorTabIdx = -1;

    // Config tabs
    QTabWidget*     m_configTabs    = nullptr;

    // Raw JSON viewer (version-level)
    QCheckBox*      m_diffToggle    = nullptr;
    QPlainTextEdit* m_configViewer  = nullptr;

    // Action buttons
    QPushButton*    m_newVersionBtn = nullptr;
    QPushButton*    m_publishBtn    = nullptr;
    QPushButton*    m_archiveBtn    = nullptr;
    QPushButton*    m_useInLiveBtn  = nullptr;
    QPushButton*    m_openBtBtn     = nullptr;

    QString         m_currentStrategyId;
    QJsonArray      m_currentVersions;
    QJsonObject     m_workingConfig;
    bool            m_configDirty = false;
};

} // namespace StrategyMgmt

#endif // STRATEGYDETAILPANEL_H
