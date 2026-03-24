#include "PipelineConfigMutations.h"
#include "PipelineConstants.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Pipeline {

bool addBlockToPipeline(QJsonObject& pipeline,
                        const QString& category,
                        const QString& blockId,
                        const QJsonObject& defaultConfig)
{
    QLatin1StringView key = categoryKey(category);
    if (key.isEmpty())
        return false;

    QJsonObject block;
    block[Key::BlockId] = blockId;
    block[Key::Config]  = defaultConfig;

    if (categoryIsArray(category)) {
        QJsonArray arr = pipeline.value(key).toArray();
        arr.append(block);
        pipeline[key] = arr;
    } else {
        pipeline[key] = block;
    }
    return true;
}

bool removeBlockFromPipeline(QJsonObject& pipeline,
                             const QString& category,
                             int blockIndex)
{
    QLatin1StringView key = categoryKey(category);
    if (key.isEmpty())
        return false;

    if (categoryIsArray(category)) {
        QJsonArray arr = pipeline.value(key).toArray();
        if (blockIndex < 0 || blockIndex >= arr.size())
            return false;
        arr.removeAt(blockIndex);
        pipeline[key] = arr;
    } else {
        pipeline.remove(key);
    }
    return true;
}

} // namespace Pipeline
