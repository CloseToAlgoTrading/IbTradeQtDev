#include "PipelineDiagramModel.h"
#include "Pipeline/StrategyRuntimePolicy.h"
#include <QJsonArray>

static const QColor kSelectionColor(0x4C, 0xAF, 0x50);
static const QColor kAlphaColor(0x21, 0x96, 0xF3);
static const QColor kMergeColor(0x9C, 0x27, 0xB0);
static const QColor kRebalanceColor(0xFF, 0x98, 0x00);
static const QColor kRiskColor(0xF4, 0x43, 0x36);
static const QColor kExecutionColor(0x60, 0x7D, 0x8B);

PipelineDiagramModel::PipelineDiagramModel(QObject* parent)
    : QObject(parent)
{
}

void PipelineDiagramModel::setPipelineConfig(const QJsonObject& config)
{
    m_viewMode = PipelineView;
    m_config = config;
    m_blocks = buildBlocks(config);
    m_policySummary = buildPolicySummary(config);
    emit dataChanged();
}

void PipelineDiagramModel::setAccountView(const QString& accountName,
                                           const QStringList& portfolioNames)
{
    m_viewMode = AccountView;
    m_accountName = accountName;
    m_portfolioNames = portfolioNames;
    m_blocks.clear();
    emit dataChanged();
}

void PipelineDiagramModel::setPortfolioView(const QString& portfolioName,
                                             const QVector<VM::StrategyDiagram>& strategies)
{
    m_viewMode = PortfolioView;
    m_portfolioName = portfolioName;
    m_strategies = strategies;
    m_blocks.clear();
    emit dataChanged();
}

void PipelineDiagramModel::clear()
{
    m_viewMode = Empty;
    m_config = {};
    m_blocks.clear();
    m_portfolioNames.clear();
    m_strategies.clear();
    emit dataChanged();
}

QVector<VM::BlockDiagramNode> PipelineDiagramModel::buildBlocks(const QJsonObject& config)
{
    QVector<VM::BlockDiagramNode> blocks;

    if (config.contains("selection")) {
        QJsonObject sel = config["selection"].toObject();
        blocks.append({sel.value("blockId").toString("Selection"), kSelectionColor});
    }

    QJsonArray alphas = config.value("alphas").toArray();
    for (int i = 0; i < alphas.size(); ++i) {
        QString id = alphas[i].toObject().value("blockId").toString(QString("Alpha %1").arg(i));
        blocks.append({id, kAlphaColor});
    }

    if (alphas.size() > 1) {
        QString policy = config.value("mergePolicy").toString("merge");
        blocks.append({policy, kMergeColor});
    }

    if (config.contains("rebalance")) {
        QJsonObject reb = config["rebalance"].toObject();
        blocks.append({reb.value("blockId").toString("Rebalance"), kRebalanceColor});
    }

    QJsonArray risks = config.value("risks").toArray();
    for (int i = 0; i < risks.size(); ++i) {
        QString id = risks[i].toObject().value("blockId").toString(QString("Risk %1").arg(i));
        blocks.append({id, kRiskColor});
    }

    if (config.contains("execution")) {
        QJsonObject exec = config["execution"].toObject();
        blocks.append({exec.value("blockId").toString("Execution"), kExecutionColor});
    }

    return blocks;
}

QString PipelineDiagramModel::buildPolicySummary(const QJsonObject& config)
{
    QJsonObject policyObj = config.value("runtimePolicy").toObject();
    if (policyObj.isEmpty())
        return {};

    auto p = Pipeline::StrategyRuntimePolicy::fromJson(policyObj);
    if (p.isDefault())
        return {};

    return p.summary();
}

VM::StrategyDiagram PipelineDiagramModel::buildStrategyDiagram(
    const QString& name, const QJsonObject& pipelineConfig)
{
    VM::StrategyDiagram d;
    d.name = name;
    d.blocks = buildBlocks(pipelineConfig);
    d.policySummary = buildPolicySummary(pipelineConfig);
    return d;
}
