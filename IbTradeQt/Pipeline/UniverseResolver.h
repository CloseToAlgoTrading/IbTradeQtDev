#ifndef PIPELINE_UNIVERSERESOLVER_H
#define PIPELINE_UNIVERSERESOLVER_H

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace Pipeline {

struct UniverseResolutionResult {
    enum class Mode {
        ExplicitStaticSymbols,    // selection block provides a known symbol list
        RequiresExternalUniverse, // block needs upstream universe (pass-all, filter, ranker)
        DerivedFromDataset,       // symbols come from the data source
        Unsupported               // block type cannot resolve symbols pre-run
    };
    Mode mode = Mode::Unsupported;
    QVector<QString> symbols;
    QString reason;
};

class UniverseResolver {
public:
    // Resolves tradeable symbols from the pipeline config's selection block(s).
    // Inspects each selection block entry: if any provides a static symbol list,
    // those symbols are returned. If all blocks are pass-all or require an
    // external universe, RequiresExternalUniverse is returned.
    static UniverseResolutionResult resolve(const QJsonObject& pipelineConfig);
};

} // namespace Pipeline

#endif // PIPELINE_UNIVERSERESOLVER_H
