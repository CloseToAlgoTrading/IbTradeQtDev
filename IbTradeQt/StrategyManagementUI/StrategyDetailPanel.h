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

namespace StrategyMgmt {

// Right panel of the Strategy Management tab showing metadata, version list,
// version detail, and action buttons for the currently selected strategy.
class StrategyDetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StrategyDetailPanel(QWidget* parent = nullptr);

    // Show details for a strategy. versions is the result of listStrategyVersions().
    void showStrategy(const QJsonObject& catalogEntry, const QJsonArray& versions);

    // Clear the panel (no strategy selected).
    void clear();

signals:
    // Metadata edit confirmed by user
    void metadataChanged(const QString& strategyId,
                         const QString& name,
                         const QString& description,
                         const QString& tags,
                         const QString& lifecycleState);

    void newVersionRequested(const QString& strategyId);
    void publishRequested(const QString& strategyId, const QString& versionId);
    void archiveRequested(const QString& strategyId);
    void useInLiveRequested(const QString& strategyId, const QString& versionId);
    void openInBacktestRequested(const QString& strategyId, const QString& versionId);

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

    // Header / metadata
    QLineEdit*      m_nameEdit      = nullptr;
    QLabel*         m_kindLabel     = nullptr;
    QComboBox*      m_statusCombo   = nullptr;
    QTextEdit*      m_descEdit      = nullptr;
    QLineEdit*      m_tagsEdit      = nullptr;
    QPushButton*    m_saveMetaBtn   = nullptr;

    // Version table
    QTableWidget*   m_versionTable  = nullptr;

    // Version detail viewer
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
};

} // namespace StrategyMgmt

#endif // STRATEGYDETAILPANEL_H
