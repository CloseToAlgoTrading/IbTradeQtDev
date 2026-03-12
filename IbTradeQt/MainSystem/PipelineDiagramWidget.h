#ifndef PIPELINEDIAGRAMWIDGET_H
#define PIPELINEDIAGRAMWIDGET_H

#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QVector>

struct StrategyDiagramInfo {
    QString name;
    int blockCount = 0;
    QJsonObject pipelineConfig;
};

class PipelineDiagramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PipelineDiagramWidget(QWidget* parent = nullptr);

    void setPipelineConfig(const QJsonObject& config);
    void setAccountView(const QString& accountName, const QStringList& portfolioNames);
    void setPortfolioView(const QString& portfolioName, const QVector<StrategyDiagramInfo>& strategies);
    void clear();

    QSize minimumSizeHint() const override { return {400, 80}; }
    QSize sizeHint() const override { return {600, 120}; }

public slots:
    void updateFromConfig(const QJsonObject& config) { setPipelineConfig(config); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    enum ViewMode { Empty, PipelineView, AccountView, PortfolioView };

    struct BlockInfo {
        QString label;
        QColor color;
    };

    ViewMode m_viewMode = Empty;
    QVector<BlockInfo> m_blocks;
    QJsonObject m_config;

    QString m_accountName;
    QStringList m_portfolioNames;

    QString m_portfolioName;
    QVector<StrategyDiagramInfo> m_strategies;

    void rebuildBlocks();
    void paintPipeline(QPainter& p);
    void paintAccountView(QPainter& p);
    void paintPortfolioView(QPainter& p);
};

#endif // PIPELINEDIAGRAMWIDGET_H
