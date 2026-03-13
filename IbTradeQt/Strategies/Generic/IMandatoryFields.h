#ifndef IMANDATORYFIELDS_H
#define IMANDATORYFIELDS_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSet>
#include "ModelType.h"

class CGenericModelApi;

class IMandatoryFields
{
public:
    virtual ~IMandatoryFields() = default;

    // --- Registration ---

    // Register a mandatory parameter. Key cannot be removed by
    // setParameters() or clear. Value IS editable.
    virtual void registerMandatoryParam(const QString& key,
                                        const QVariant& defaultValue) = 0;

    // Register a mandatory parameter that participates in
    // resolvedParam() inheritance (walks up parent chain).
    // Only use for params that share the same key across levels
    // and semantically inherit (e.g., BenchmarkRef).
    virtual void registerInheritableParam(const QString& key,
                                          const QVariant& defaultValue) = 0;

    virtual void registerMandatoryInfo(const QString& key,
                                       const QVariant& defaultValue) = 0;

    virtual void registerMandatoryAssetField(const QString& key,
                                              const QVariant& defaultValue) = 0;

    // --- Query ---
    virtual const QSet<QString>& mandatoryParamKeys() const = 0;
    virtual const QSet<QString>& mandatoryInfoKeys() const = 0;
    virtual const QSet<QString>& mandatoryAssetFieldKeys() const = 0;

    // --- Cross-level access ---

    // Find ancestor by ModelType. Strategy calling
    // findAncestor(ACCOUNT) returns the owning Account.
    virtual CGenericModelApi* findAncestor(ModelType type) const = 0;

    // Resolve an inheritable param: returns own value if non-empty,
    // else walks up parent chain. ONLY works for keys registered
    // via registerInheritableParam(). For non-inheritable keys,
    // returns own value without walking up.
    virtual QVariant resolvedParam(const QString& key,
                                   const QVariant& fallback = {}) const = 0;

    // Sum a numeric info field from direct children.
    // For additive fields only (PnL, counts).
    virtual double aggregateChildInfo(const QString& key) const = 0;

    // Create asset entry with all mandatory asset fields defaulted,
    // then overlay with provided values.
    virtual QVariantMap createAssetEntry(
        const QVariantMap& values = {}) const = 0;
};

#endif // IMANDATORYFIELDS_H
