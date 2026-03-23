#include "SemanticPipelineChain.h"

#include "SemanticModelDataMapper.h"
#include <QMap>

namespace Pipeline {

ModelDataList mergeModelDataWithTickSignals(
    const ModelDataList& modelAfterAlpha,
    const QVector<Signal>& tickSignals,
    bool combineTick,
    const QString& correlationId,
    const QString& semanticAlphaBlockId)
{
    if (!combineTick || tickSignals.isEmpty())
        return modelAfterAlpha;

    QVector<Signal> fromModel =
        SemanticMapping::signalsFromModelData(modelAfterAlpha, correlationId, semanticAlphaBlockId);

    QMap<QString, Signal> bySymbol;
    for (const auto& s : fromModel) {
        if (!s.symbol.isEmpty())
            bySymbol[s.symbol] = s;
    }
    for (const auto& s : tickSignals) {
        Signal t = s;
        t.correlationId = correlationId;
        if (!t.symbol.isEmpty())
            bySymbol[t.symbol] = t;
    }

    QVector<Signal> merged;
    merged.reserve(bySymbol.size());
    for (auto it = bySymbol.begin(); it != bySymbol.end(); ++it)
        merged.append(it.value());

    return SemanticMapping::modelDataFromSignals(merged);
}

} // namespace Pipeline
