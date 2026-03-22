#ifndef PIPELINE_BLOCKGRAPHSERIALIZER_H
#define PIPELINE_BLOCKGRAPHSERIALIZER_H

#include <QJsonObject>
#include <QString>
#include "StrategyPipelineRunner.h"
#include "BlockRegistry.h"

namespace Pipeline {

class BlockGraphSerializer {
public:
    static QJsonObject serialize(const BlockGraph& graph);

    static Expected<BlockGraph, Error> deserialize(
        const QJsonObject& json, BlockRegistry& registry);

    static bool saveToFile(const BlockGraph& graph, const QString& filePath);

    static Expected<BlockGraph, Error> loadFromFile(
        const QString& filePath, BlockRegistry& registry);

private:
    static QJsonObject serializeLevel(const BlockGraph::LevelBlocks& level);

    static Expected<BlockGraph::LevelBlocks, Error> deserializeLevel(
        const QJsonObject& json, BlockRegistry& registry);
};

} // namespace Pipeline

#endif // PIPELINE_BLOCKGRAPHSERIALIZER_H
