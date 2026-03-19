#ifndef PIPELINE_UNIVERSERESOLVER_H
#define PIPELINE_UNIVERSERESOLVER_H

#include <QJsonObject>
#include <QJsonArray>
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
    static UniverseResolutionResult resolve(const QJsonObject& pipelineConfig) {
        UniverseResolutionResult result;

        QJsonArray selectionConfigs = pipelineConfig.value("selection").toArray();

        if (selectionConfigs.isEmpty()) {
            result.mode = UniverseResolutionResult::Mode::RequiresExternalUniverse;
            result.reason = "No selection blocks configured";
            return result;
        }

        QVector<QString> allStaticSymbols;

        for (const auto& selVal : selectionConfigs) {
            QJsonObject selCfg = selVal.toObject();
            QString blockId = selCfg.value("blockId").toString();
            QJsonObject blockConfig = selCfg.value("config").toObject();

            if (blockId == "static-list-selection" || blockId == "static-list") {
                QJsonArray symbolsArr = blockConfig.value("symbols").toArray();
                for (const auto& s : symbolsArr) {
                    const QString sym = s.toString();
                    if (!sym.isEmpty() && !allStaticSymbols.contains(sym))
                        allStaticSymbols.append(sym);
                }
            }
        }

        if (!allStaticSymbols.isEmpty()) {
            result.mode = UniverseResolutionResult::Mode::ExplicitStaticSymbols;
            result.symbols = allStaticSymbols;
            result.reason = QString("Static list: %1 symbols from selection config")
                                .arg(allStaticSymbols.size());
            return result;
        }

        result.mode = UniverseResolutionResult::Mode::RequiresExternalUniverse;
        result.reason = "Selection blocks require an external universe (pass-all or filter)";
        return result;
    }
};

} // namespace Pipeline

#endif // PIPELINE_UNIVERSERESOLVER_H
