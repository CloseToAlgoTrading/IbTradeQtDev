#ifndef BLOCKWORKSPACE_H
#define BLOCKWORKSPACE_H

#include <QWidget>
#include <QJsonObject>

class QVBoxLayout;
class QTabWidget;
class QFormLayout;
class QLabel;
class QLineEdit;
class WorkspaceHeader;
class CPipelineStrategyAdapter;

class BlockWorkspace : public QWidget
{
    Q_OBJECT
public:
    explicit BlockWorkspace(QWidget* parent = nullptr);

    void setBlockContext(CPipelineStrategyAdapter* adapter,
                         const QString& category,
                         const QString& blockId,
                         const QString& jsonKey,
                         int arrayIndex);
    void clearContext();

signals:
    void configChanged(const QJsonObject& newPipelineConfig);

private:
    void buildPropertiesTab();
    void buildInfoTab();
    void buildAssetsTab();

    void refreshProperties();
    void refreshInfo();
    void refreshAssets();

    void writeBackConfig();

    WorkspaceHeader* m_header       = nullptr;
    QTabWidget*      m_tabWidget    = nullptr;

    QWidget*     m_propertiesWidget = nullptr;
    QFormLayout* m_propertiesForm   = nullptr;

    QWidget*     m_infoWidget       = nullptr;
    QFormLayout* m_infoForm         = nullptr;

    QWidget*     m_assetsWidget     = nullptr;
    QFormLayout* m_assetsForm       = nullptr;
    QLineEdit*   m_symbolsEdit      = nullptr;

    CPipelineStrategyAdapter* m_adapter = nullptr;
    QString m_category;
    QString m_blockId;
    QString m_jsonKey;
    int     m_arrayIndex = -1;

    QJsonObject currentBlockEntry() const;
};

#endif // BLOCKWORKSPACE_H
