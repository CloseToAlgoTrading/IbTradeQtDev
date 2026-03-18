#ifndef PIPELINECONFIGEDITOR_H
#define PIPELINECONFIGEDITOR_H

#include <QWidget>
#include <QJsonObject>

class QTreeWidget;
class QTreeWidgetItem;
class QFormLayout;
class QScrollArea;
class QLabel;
class QSplitter;

// Standalone pipeline config editor that works purely on a QJsonObject.
// Shows a block tree on the left (organized by category) and editable
// parameter fields on the right.  Used by both Backtest and Strategy Management.
class PipelineConfigEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PipelineConfigEditor(QWidget* parent = nullptr);

    void setPipelineConfig(const QJsonObject& config);
    QJsonObject pipelineConfig() const { return m_config; }

    void setReadOnly(bool readOnly);
    void clear();

signals:
    void configChanged(const QJsonObject& newConfig);

private slots:
    void onBlockSelected();

private:
    void buildUi();
    void rebuildTree();
    void showBlockParams(const QString& category, const QString& jsonKey,
                         bool isArray, int arrayIndex);
    void clearParamPanel();

    QSplitter*    m_splitter    = nullptr;
    QTreeWidget*  m_blockTree   = nullptr;
    QScrollArea*  m_paramScroll = nullptr;
    QWidget*      m_paramWidget = nullptr;
    QFormLayout*  m_paramForm   = nullptr;
    QLabel*       m_emptyLabel  = nullptr;

    QJsonObject   m_config;
    bool          m_readOnly = false;
};

#endif // PIPELINECONFIGEDITOR_H
