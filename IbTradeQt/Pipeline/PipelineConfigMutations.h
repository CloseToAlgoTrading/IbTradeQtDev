#ifndef PIPELINECONFIGMUTATIONS_H
#define PIPELINECONFIGMUTATIONS_H

#include <QJsonObject>
#include <QString>

namespace Pipeline {

// Pure JSON transforms for pipelineConfig objects (strategy catalog / workspace).
// Single source of truth for add/remove block structure used by Strategy Management.

bool addBlockToPipeline(QJsonObject& pipeline,
                        const QString& category,
                        const QString& blockId,
                        const QJsonObject& defaultConfig);

bool removeBlockFromPipeline(QJsonObject& pipeline,
                             const QString& category,
                             int blockIndex);

} // namespace Pipeline

#endif // PIPELINECONFIGMUTATIONS_H
