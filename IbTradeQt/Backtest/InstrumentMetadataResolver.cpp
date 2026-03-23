#include "Backtest/InstrumentMetadataResolver.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/InstrumentNormalization.h"
#include "DB/dbquery.h"
#include "Strategies/Generic/mandatoryFieldKeys.h"
#include <QSqlDatabase>
#include <QSqlQuery>

namespace Backtest {

EffectiveInstrumentProfile InstrumentMetadataResolver::resolve(const QString& dbConnectionName,
                                                               const QString& rawSymbol,
                                                               const QString& providerId,
                                                               const QVariantMap& strategyAssetEntry)
{
    EffectiveInstrumentProfile p;
    const QString norm = normalizeProviderSymbol(providerId, rawSymbol);
    p.providerSymbol   = norm;
    p.providerId       = providerId;

    const QString co =
        strategyAssetEntry.value(AssetFields::Position::ClassificationOverride).toString().trimmed();
    if (!co.isEmpty()) {
        AssetKind ok{};
        if (assetKindFromString(co, &ok)) {
            p.classificationOverrideAssetKind = ok;
            p.effectiveAssetKind              = ok;
            p.provenance = InstrumentClassificationProvenance::ClassificationOverride;
            p.isInferred = false;
            const QString sp =
                strategyAssetEntry.value(AssetFields::Position::SessionPolicy).toString().trimmed();
            if (!sp.isEmpty())
                p.sessionPolicy = sp;
            return p;
        }
    }

    const QString sp =
        strategyAssetEntry.value(AssetFields::Position::SessionPolicy).toString().trimmed();
    if (!sp.isEmpty())
        p.sessionPolicy = sp;

    if (!dbConnectionName.isEmpty() && QSqlDatabase::database(dbConnectionName).isOpen()) {
        QSqlQuery q = query_selectInstrumentMetadata(norm, providerId, dbConnectionName);
        if (q.exec() && q.next()) {
            const QString aks = q.value(QStringLiteral("assetKind")).toString();
            AssetKind           pk{};
            if (assetKindFromString(aks, &pk) && pk != AssetKind::Unknown) {
                p.providerAssetKind  = pk;
                p.effectiveAssetKind = pk;
                p.provenance         = InstrumentClassificationProvenance::ProviderMetadata;
                p.isInferred         = false;
                return p;
            }
        }
    }

    const AssetKind inf = inferAssetKindFromSymbolHeuristic(rawSymbol);
    p.effectiveAssetKind = inf;
    p.provenance         = InstrumentClassificationProvenance::InferredHeuristic;
    p.isInferred         = true;
    return p;
}

} // namespace Backtest
