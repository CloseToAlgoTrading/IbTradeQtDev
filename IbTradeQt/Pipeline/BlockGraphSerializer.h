#ifndef PIPELINE_BLOCKGRAPHSERIALIZER_H
#define PIPELINE_BLOCKGRAPHSERIALIZER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include "StrategyPipelineRunner.h"
#include "BlockRegistry.h"

namespace Pipeline {

class BlockGraphSerializer {
public:
    static QJsonObject serialize(const BlockGraph& graph) {
        QJsonObject json;

        QJsonArray selectionIds;
        for (auto* block : graph.selectionBlocks) {
            QJsonObject blockObj;
            blockObj["id"] = block->id();
            blockObj["config"] = block->config();
            selectionIds.append(blockObj);
        }
        json["selectionBlocks"] = selectionIds;

        QJsonArray alphaIds;
        for (auto* block : graph.alphaBlocks) {
            QJsonObject blockObj;
            blockObj["id"] = block->id();
            blockObj["config"] = block->config();
            alphaIds.append(blockObj);
        }
        json["alphaBlocks"] = alphaIds;

        if (graph.mergePolicy) {
            json["mergePolicy"] = graph.mergePolicy->id();
        }

        json["strategyLevel"] = serializeLevel(graph.strategyLevel);
        json["portfolioLevel"] = serializeLevel(graph.portfolioLevel);
        json["accountLevel"] = serializeLevel(graph.accountLevel);

        if (graph.executionBlock) {
            QJsonObject execObj;
            execObj["id"] = graph.executionBlock->id();
            execObj["config"] = graph.executionBlock->config();
            json["executionBlock"] = execObj;
        }

        json["config"] = graph.config;

        return json;
    }

    static Expected<BlockGraph, Error> deserialize(
        const QJsonObject& json, BlockRegistry& registry)
    {
        BlockGraph graph;

        auto selectionArr = json["selectionBlocks"].toArray();
        for (const auto& val : selectionArr) {
            auto obj = val.toObject();
            auto result = registry.createBlock(obj["id"].toString());
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
            block->setConfig(obj["config"].toObject());
            graph.selectionBlocks.append(block);
        }

        auto alphaArr = json["alphaBlocks"].toArray();
        for (const auto& val : alphaArr) {
            auto obj = val.toObject();
            auto result = registry.createBlock(obj["id"].toString());
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
            block->setConfig(obj["config"].toObject());
            graph.alphaBlocks.append(block);
        }

        if (json.contains("mergePolicy")) {
            auto result = registry.createBlock(json["mergePolicy"].toString());
            if (!result) return make_unexpected(result.error());
            graph.mergePolicy = qobject_cast<ISignalMergePolicy*>(*result);
        }

        auto stratRes = deserializeLevel(json["strategyLevel"].toObject(), registry);
        if (!stratRes) return make_unexpected(stratRes.error());
        graph.strategyLevel = *stratRes;

        auto portRes = deserializeLevel(json["portfolioLevel"].toObject(), registry);
        if (!portRes) return make_unexpected(portRes.error());
        graph.portfolioLevel = *portRes;

        auto acctRes = deserializeLevel(json["accountLevel"].toObject(), registry);
        if (!acctRes) return make_unexpected(acctRes.error());
        graph.accountLevel = *acctRes;

        if (json.contains("executionBlock")) {
            auto execObj = json["executionBlock"].toObject();
            auto result = registry.createBlock(execObj["id"].toString());
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
            block->setConfig(execObj["config"].toObject());
            graph.executionBlock = block;
        }

        graph.config = json["config"].toObject();

        return graph;
    }

    static bool saveToFile(const BlockGraph& graph, const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QJsonDocument doc(serialize(graph));
        file.write(doc.toJson(QJsonDocument::Indented));
        return true;
    }

    static Expected<BlockGraph, Error> loadFromFile(
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

private:
    static QJsonObject serializeLevel(const BlockGraph::LevelBlocks& level) {
        QJsonObject obj;
        if (level.rebalance) {
            QJsonObject rebalObj;
            rebalObj["id"] = level.rebalance->id();
            rebalObj["config"] = level.rebalance->config();
            obj["rebalance"] = rebalObj;
        }
        QJsonArray riskArr;
        for (auto* risk : level.risks) {
            QJsonObject riskObj;
            riskObj["id"] = risk->id();
            riskObj["config"] = risk->config();
            riskArr.append(riskObj);
        }
        if (!riskArr.isEmpty()) {
            obj["risks"] = riskArr;
        }
        return obj;
    }

    static Expected<BlockGraph::LevelBlocks, Error> deserializeLevel(
        const QJsonObject& json, BlockRegistry& registry)
    {
        BlockGraph::LevelBlocks level;

        if (json.contains("rebalance")) {
            auto rebalObj = json["rebalance"].toObject();
            auto result = registry.createBlock(rebalObj["id"].toString());
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
            level.rebalance->setConfig(rebalObj["config"].toObject());
        }

        auto riskArr = json["risks"].toArray();
        for (const auto& val : riskArr) {
            auto riskObj = val.toObject();
            auto result = registry.createBlock(riskObj["id"].toString());
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
            risk->setConfig(riskObj["config"].toObject());
            level.risks.append(risk);
        }

        return level;
    }
};

} // namespace Pipeline

#endif // PIPELINE_BLOCKGRAPHSERIALIZER_H
