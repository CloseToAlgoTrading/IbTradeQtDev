#ifndef PIPELINEDIAGRAMWIDGET_H
#define PIPELINEDIAGRAMWIDGET_H

#include <QWidget>
#include <QJsonObject>
#include <QVector>
#include "ViewModels.h"

class PipelineDiagramModel;

struct StrategyDiagramInfo {
    QString name;
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

    PipelineDiagramModel* diagramModel() const { return m_model; }

    QSize minimumSizeHint() const override { return {400, 80}; }
    QSize sizeHint() const override { return {600, 120}; }

public slots:
    void updateFromConfig(const QJsonObject& config) { setPipelineConfig(config); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    PipelineDiagramModel* m_model;

    void paintPipeline(QPainter& p);
    void paintAccountView(QPainter& p);
    void paintPortfolioView(QPainter& p);
};

#endif // PIPELINEDIAGRAMWIDGET_H
