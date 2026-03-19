#ifndef BLOCKINSPECTORPANEL_H
#define BLOCKINSPECTORPANEL_H

#include <QWidget>
#include <QJsonObject>

class QFormLayout;
class QScrollArea;
class QLabel;
class QPlainTextEdit;
class QCheckBox;
class BlockInspectorPresenter;

// Single source of truth for block parameter editing, block info display,
// and JSON diff.  Driven externally by tree selection -- does not contain
// its own tree.  Used by all three surfaces (Live, Backtest, Strategy Mgmt).
class BlockInspectorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BlockInspectorPanel(QWidget* parent = nullptr);

    void showBlock(const QJsonObject& pipelineConfig,
                   const QString& category,
                   const QString& jsonKey,
                   bool isArray, int arrayIndex);

    void setReadOnly(bool readOnly);
    void setDiffPanelVisible(bool visible);
    void setComparisonConfig(const QJsonObject& baseline);

    void clear();

signals:
    void configChanged(const QJsonObject& updatedPipelineConfig);

private:
    void buildUi();
    void clearForm();
    void updateJsonViewer();
    QString computeDiff(const QString& current, const QString& baseline) const;

    QScrollArea*    m_paramScroll    = nullptr;
    QWidget*        m_paramWidget    = nullptr;
    QFormLayout*    m_paramForm      = nullptr;
    QLabel*         m_emptyLabel     = nullptr;

    QWidget*        m_diffContainer  = nullptr;
    QCheckBox*      m_diffToggle     = nullptr;
    QPlainTextEdit* m_jsonViewer     = nullptr;

    QJsonObject     m_config;
    QJsonObject     m_comparisonConfig;
    bool            m_readOnly = false;

    BlockInspectorPresenter* m_presenter = nullptr;
};

#endif // BLOCKINSPECTORPANEL_H
