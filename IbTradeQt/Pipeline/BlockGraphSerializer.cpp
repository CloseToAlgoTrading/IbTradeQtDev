#include "BlockGraphSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace Pipeline {

QJsonObject BlockGraphSerializer::serialize(const BlockGraph& graph)
{
    QJsonObject json;

    QJsonArray selectionIds;
    for (auto* block : graph.selectionBlocks) {
        QJsonObject blockObj;
        blockObj[QStringLiteral("id")] = block->id();
        blockObj[QStringLiteral("config")] = block->config();
        selectionIds.append(blockObj);
    }
    json[QStringLiteral("selectionBlocks")] = selectionIds;

    QJsonArray alphaIds;
    for (auto* block : graph.alphaBlocks) {
        QJsonObject blockObj;
        blockObj[QStringLiteral("id")] = block->id();
        blockObj[QStringLiteral("config")] = block->config();
        alphaIds.append(blockObj);
    }
    json[QStringLiteral("alphaBlocks")] = alphaIds;

    if (graph.mergePolicy) {
        json[QStringLiteral("mergePolicy")] = graph.mergePolicy->id();
    }

    json[QStringLiteral("strategyLevel")] = serializeLevel(graph.strategyLevel);
    json[QStringLiteral("portfolioLevel")] = serializeLevel(graph.portfolioLevel);
    json[QStringLiteral("accountLevel")] = serializeLevel(graph.accountLevel);

    if (graph.executionBlock) {
        QJsonObject execObj;
        execObj[QStringLiteral("id")] = graph.executionBlock->id();
        execObj[QStringLiteral("config")] = graph.executionBlock->config();
        json[QStringLiteral("executionBlock")] = execObj;
    }

    json[QStringLiteral("config")] = graph.config;

    return json;
}

Expected<BlockGraph, Error> BlockGraphSerializer::deserialize(
    const QJsonObject& json, BlockRegistry& registry)
{
    BlockGraph graph;

    auto selectionArr = json[QStringLiteral("selectionBlocks")].toArray();
    for (const auto& val : selectionArr) {
        auto obj = val.toObject();
        auto result = registry.createBlock(obj[QStringLiteral("id")].toString());
        if (!result) return make_unexpected(result.error());
        auto* block = qobject_cast<ISelectionBlock*>(*result);
        if (!block) {
            delete *result;
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block is not an ISelectionBlock",
                "BlockGraphSerializer::deserialize"
            });
        }
        block->setConfig(obj[QStringLiteral("config")].toObject());
        graph.selectionBlocks.append(block);
    }

    auto alphaArr = json[QStringLiteral("alphaBlocks")].toArray();
    for (const auto& val : alphaArr) {
        auto obj = val.toObject();
        auto result = registry.createBlock(obj[QStringLiteral("id")].toString());
        if (!result) return make_unexpected(result.error());
        auto* block = qobject_cast<IAlphaBlock*>(*result);
        if (!block) {
            delete *result;
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block is not an IAlphaBlock",
                "BlockGraphSerializer::deserialize"
            });
        }
        block->setConfig(obj[QStringLiteral("config")].toObject());
        graph.alphaBlocks.append(block);
    }

    if (json.contains(QStringLiteral("mergePolicy"))) {
        auto result = registry.createBlock(json[QStringLiteral("mergePolicy")].toString());
        if (!result) return make_unexpected(result.error());
        graph.mergePolicy = qobject_cast<ISignalMergePolicy*>(*result);
    }

    auto stratRes = deserializeLevel(json[QStringLiteral("strategyLevel")].toObject(), registry);
    if (!stratRes) return make_unexpected(stratRes.error());
    graph.strategyLevel = *stratRes;

    auto portRes = deserializeLevel(json[QStringLiteral("portfolioLevel")].toObject(), registry);
    if (!portRes) return make_unexpected(portRes.error());
    graph.portfolioLevel = *portRes;

    auto acctRes = deserializeLevel(json[QStringLiteral("accountLevel")].toObject(), registry);
    if (!acctRes) return make_unexpected(acctRes.error());
    graph.accountLevel = *acctRes;

    if (json.contains(QStringLiteral("executionBlock"))) {
        auto execObj = json[QStringLiteral("executionBlock")].toObject();
        auto result = registry.createBlock(execObj[QStringLiteral("id")].toString());
        if (!result) return make_unexpected(result.error());
        auto* block = qobject_cast<IExecutionBlock*>(*result);
        if (!block) {
            delete *result;
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block is not an IExecutionBlock",
                "BlockGraphSerializer::deserialize"
            });
        }
        block->setConfig(execObj[QStringLiteral("config")].toObject());
        graph.executionBlock = block;
    }

    graph.config = json[QStringLiteral("config")].toObject();

    return graph;
}

bool BlockGraphSerializer::saveToFile(const BlockGraph& graph, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QJsonDocument doc(serialize(graph));
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

Expected<BlockGraph, Error> BlockGraphSerializer::loadFromFile(
    const QString& filePath, BlockRegistry& registry)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            "Cannot open file: " + filePath.toStdString(),
            "BlockGraphSerializer::loadFromFile"
        });
    }
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        return make_unexpected(Error{
            ErrorCode::ConfigurationError,
            "JSON parse error: " + parseErr.errorString().toStdString(),
            "BlockGraphSerializer::loadFromFile"
        });
    }
    return deserialize(doc.object(), registry);
}

QJsonObject BlockGraphSerializer::serializeLevel(const BlockGraph::LevelBlocks& level)
{
    QJsonObject obj;
    if (level.rebalance) {
        QJsonObject rebalObj;
        rebalObj[QStringLiteral("id")] = level.rebalance->id();
        rebalObj[QStringLiteral("config")] = level.rebalance->config();
        obj[QStringLiteral("rebalance")] = rebalObj;
    }
    QJsonArray riskArr;
    for (auto* risk : level.risks) {
        QJsonObject riskObj;
        riskObj[QStringLiteral("id")] = risk->id();
        riskObj[QStringLiteral("config")] = risk->config();
        riskArr.append(riskObj);
    }
    if (!riskArr.isEmpty()) {
        obj[QStringLiteral("risks")] = riskArr;
    }
    return obj;
}

Expected<BlockGraph::LevelBlocks, Error> BlockGraphSerializer::deserializeLevel(
    const QJsonObject& json, BlockRegistry& registry)
{
    BlockGraph::LevelBlocks level;

    if (json.contains(QStringLiteral("rebalance"))) {
        auto rebalObj = json[QStringLiteral("rebalance")].toObject();
        auto result = registry.createBlock(rebalObj[QStringLiteral("id")].toString());
        if (!result) return make_unexpected(result.error());
        level.rebalance = qobject_cast<IRebalanceBlock*>(*result);
        if (!level.rebalance) {
            delete *result;
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block is not an IRebalanceBlock",
                "BlockGraphSerializer::deserializeLevel"
            });
        }
        level.rebalance->setConfig(rebalObj[QStringLiteral("config")].toObject());
    }

    auto riskArr = json[QStringLiteral("risks")].toArray();
    for (const auto& val : riskArr) {
        auto riskObj = val.toObject();
        auto result = registry.createBlock(riskObj[QStringLiteral("id")].toString());
        if (!result) return make_unexpected(result.error());
        auto* risk = qobject_cast<IRiskBlock*>(*result);
        if (!risk) {
            delete *result;
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block is not an IRiskBlock",
                "BlockGraphSerializer::deserializeLevel"
            });
        }
        risk->setConfig(riskObj[QStringLiteral("config")].toObject());
        level.risks.append(risk);
    }

    return level;
}

} // namespace Pipeline
