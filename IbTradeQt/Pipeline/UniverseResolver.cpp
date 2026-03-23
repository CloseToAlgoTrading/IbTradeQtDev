#include "UniverseResolver.h"

#include <QJsonArray>

namespace Pipeline {

UniverseResolutionResult UniverseResolver::resolve(const QJsonObject& pipelineConfig)
{
    UniverseResolutionResult result;

    QJsonArray selectionConfigs = pipelineConfig.value(QStringLiteral("selection")).toArray();

    if (selectionConfigs.isEmpty()) {
        result.mode = UniverseResolutionResult::Mode::RequiresExternalUniverse;
        result.reason = QStringLiteral("No selection blocks configured");
        return result;
    }

    QVector<QString> allStaticSymbols;

    for (const auto& selVal : selectionConfigs) {
        QJsonObject selCfg = selVal.toObject();
        QString blockId = selCfg.value(QStringLiteral("blockId")).toString();
        QJsonObject blockConfig = selCfg.value(QStringLiteral("config")).toObject();

        if (blockId == QStringLiteral("static-list-selection") || blockId == QStringLiteral("static-list")) {
            QJsonArray symbolsArr = blockConfig.value(QStringLiteral("symbols")).toArray();
            for (const auto& s : symbolsArr) {
                const QString sym = s.toString().trimmed().toUpper();
                if (!sym.isEmpty() && !allStaticSymbols.contains(sym))
                    allStaticSymbols.append(sym);
            }
        }
    }

    if (!allStaticSymbols.isEmpty()) {
        result.mode = UniverseResolutionResult::Mode::ExplicitStaticSymbols;
        result.symbols = allStaticSymbols;
        result.reason = QStringLiteral("Static list: %1 symbols from selection config")
                            .arg(allStaticSymbols.size());
        return result;
    }

    result.mode = UniverseResolutionResult::Mode::RequiresExternalUniverse;
    result.reason = QStringLiteral("Selection blocks require an external universe (pass-all or filter)");
    return result;
}

} // namespace Pipeline
