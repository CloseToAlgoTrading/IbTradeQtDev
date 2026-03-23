#ifndef BACKTEST_INSTRUMENTCLASSIFICATION_H
#define BACKTEST_INSTRUMENTCLASSIFICATION_H

#include <QString>
#include <optional> // IWYU: std::optional

namespace Backtest {

/// Coarse instrument class only — no region/session in the enum.
enum class AssetKind : quint8 {
    Unknown = 0,
    Equity,
    Etf,
    Forex,
    Crypto,
    Future,
    Option,
    Index,
    Fund,
    Bond,
};

/// Why the effective kind was chosen (v1 set).
enum class InstrumentClassificationProvenance : quint8 {
    Indeterminate = 0, ///< Provenance unknown / effective kind is Unknown
    ClassificationOverride,
    ProviderMetadata,
    InferredHeuristic,
};

/// Resolver output — single struct for UI, session, and strategy.
struct EffectiveInstrumentProfile {
    QString     providerSymbol;
    QString     providerId; ///< e.g. yahoo, ib — active for current resolution mode

    std::optional<AssetKind> providerAssetKind;               ///< from InstrumentMetadata row
    std::optional<AssetKind> classificationOverrideAssetKind; ///< strategy-scoped

    AssetKind effectiveAssetKind = AssetKind::Unknown; ///< always set (use Unknown if unresolved)

    std::optional<QString> sessionPolicy; ///< v1 optional opaque id / label for session handling

    InstrumentClassificationProvenance provenance = InstrumentClassificationProvenance::Indeterminate;
    bool                               isInferred = false;
};

QString assetKindToString(AssetKind k);
bool    assetKindFromString(const QString& s, AssetKind* out);

} // namespace Backtest

#endif
