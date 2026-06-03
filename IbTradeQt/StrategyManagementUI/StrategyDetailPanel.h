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
class QStackedWidget;
class QWidget;
class QAction;
class BlockInspectorPanel;
class RuntimePolicyEditor;
class StrategyDetailPresenter;

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

    QTableWidget* versionTable() const { return m_versionTable; }

    QJsonObject workingConfig() const { return m_workingConfig; }
    bool isConfigDirty() const { return m_configDirty; }
    QString currentStrategyId() const { return m_currentStrategyId; }
    bool currentVersionPublished() const;
    void selectVersionRow(int row);
    bool selectVersionById(const QString& versionId);

    void loadVersionAtRow(int row);
    void resetWorkingToSelectedVersion();
    void setWorkingPipelineConfig(const QJsonObject& pipelineConfig);
    void setSelectedBlockContext(const QString& category, const QString& blockName);

signals:
    void metadataChanged(const QString& strategyId,
                         const QString& name,
                         const QString& description,
                         const QString& tags,
                         const QString& lifecycleState);

    void newVersionRequested(const QString& strategyId,
                             const QJsonObject& pipelineConfig);
    void publishRequested(const QString& strategyId, const QString& versionId);
    void unpublishRequested(const QString& strategyId, const QString& versionId);
    void deleteVersionRequested(const QString& strategyId, const QString& versionId);
    void versionLifecycleChanged(const QString& strategyId,
                                 const QString& versionId,
                                 const QString& lifecycleState);
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

    void versionRowChangeRequested(int newRow, int previousRow);
    void visibleVersionChanged(const QString& strategyId,
                               const QString& versionId,
                               const QString& versionLabel,
                               const QString& lifecycleState,
                               const QJsonObject& pipelineConfig);

private slots:
    void onVersionCurrentCellChanged(int currentRow, int currentColumn,
                                     int previousRow, int previousColumn);
    void onSaveMetadata();
    void onNewVersion();
    void onPublish();
    void onDeleteVersion();
    void onArchive();
    void onUseInLive();
    void onOpenInBacktest();

private:
    void buildUi();
    QString selectedVersionId() const;
    void updateVersionActionState();
    void updateNewVersionButtonText();
    void updateVersionEditLock();
    void showVersionInspector();
    void markDirty();
    bool selectedVersionPublished() const;
    QString selectedVersionLabel() const;
    QString versionIdForRow(int row) const;
    QString versionLabelForRow(int row) const;

    // Metadata
    QLineEdit*      m_nameEdit      = nullptr;
    QLabel*         m_lifecycleBadge = nullptr;
    QTextEdit*      m_descEdit      = nullptr;
    QLineEdit*      m_tagsEdit      = nullptr;
    QPushButton*    m_saveMetaBtn   = nullptr;

    // Version table
    QTableWidget*   m_versionTable  = nullptr;

    // Scope inspector
    QLabel*         m_inspectorTitleLabel = nullptr;
    QStackedWidget* m_inspectorStack = nullptr;
    QWidget*        m_versionInspectorPage = nullptr;
    QWidget*        m_blockInspectorPage = nullptr;

    // Block inspector
    BlockInspectorPanel*  m_inspector     = nullptr;

    // Runtime policy editor
    RuntimePolicyEditor*  m_policyEditor  = nullptr;

    // Advanced JSON viewer (version-level)
    QCheckBox*      m_showAdvancedJsonCheck = nullptr;
    QCheckBox*      m_diffToggle    = nullptr;
    QWidget*        m_jsonPanel     = nullptr;
    QPlainTextEdit* m_configViewer  = nullptr;

    // Action buttons
    QPushButton*    m_newVersionBtn = nullptr;
    QPushButton*    m_publishBtn    = nullptr;
    QPushButton*    m_unpublishBtn  = nullptr;
    QAction*        m_deleteVersionAction = nullptr;
    QAction*        m_archiveAction = nullptr;
    QPushButton*    m_moreActionsBtn = nullptr;
    QPushButton*    m_useInLiveBtn  = nullptr;
    QPushButton*    m_openBtBtn     = nullptr;

    QString         m_currentStrategyId;
    QString         m_currentStrategyLifecycle = QStringLiteral("draft");
    QString         m_blockInspectorTitle;
    QJsonArray      m_currentVersions;
    QJsonObject     m_workingConfig;
    bool            m_configDirty = false;
    bool            m_suppressVersionNav = false;

    StrategyDetailPresenter* m_detailPresenter = nullptr;
};

} // namespace StrategyMgmt

#endif // STRATEGYDETAILPANEL_H
