#ifndef PIPELINEDIAGRAMMODEL_H
#define PIPELINEDIAGRAMMODEL_H

#include <QObject>
#include <QJsonObject>
#include <QVector>
#include <QStringList>
#include "ViewModels.h"

class PipelineDiagramModel : public QObject
{
    Q_OBJECT
public:
    explicit PipelineDiagramModel(QObject* parent = nullptr);

    enum ViewMode { Empty, PipelineView, AccountView, PortfolioView };

    void setPipelineConfig(const QJsonObject& config);
    void setAccountView(const QString& accountName, const QStringList& portfolioNames);
    void setPortfolioView(const QString& portfolioName,
                          const QVector<VM::StrategyDiagram>& strategies);
    void clear();

    ViewMode viewMode() const { return m_viewMode; }
    QVector<VM::BlockDiagramNode> blocks() const { return m_blocks; }
    QString policySummary() const { return m_policySummary; }
    QJsonObject config() const { return m_config; }

    QString accountName() const { return m_accountName; }
    QStringList portfolioNames() const { return m_portfolioNames; }

    QString portfolioName() const { return m_portfolioName; }
    QVector<VM::StrategyDiagram> strategies() const { return m_strategies; }

    static QVector<VM::BlockDiagramNode> buildBlocks(const QJsonObject& config);
    static QString buildPolicySummary(const QJsonObject& config);
    static VM::StrategyDiagram buildStrategyDiagram(const QString& name,
                                                     const QJsonObject& pipelineConfig);

signals:
    void dataChanged();

private:
    ViewMode m_viewMode = Empty;
    QVector<VM::BlockDiagramNode> m_blocks;
    QJsonObject m_config;
    QString m_policySummary;

    QString m_accountName;
    QStringList m_portfolioNames;

    QString m_portfolioName;
    QVector<VM::StrategyDiagram> m_strategies;
};

#endif // PIPELINEDIAGRAMMODEL_H
