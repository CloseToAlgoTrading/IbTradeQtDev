#include "Backtest/InstrumentClassification.h"

namespace Backtest {

QString assetKindToString(AssetKind k)
{
    switch (k) {
    case AssetKind::Unknown:
        return QStringLiteral("Unknown");
    case AssetKind::Equity:
        return QStringLiteral("Equity");
    case AssetKind::Etf:
        return QStringLiteral("Etf");
    case AssetKind::Forex:
        return QStringLiteral("Forex");
    case AssetKind::Crypto:
        return QStringLiteral("Crypto");
    case AssetKind::Future:
        return QStringLiteral("Future");
    case AssetKind::Option:
        return QStringLiteral("Option");
    case AssetKind::Index:
        return QStringLiteral("Index");
    case AssetKind::Fund:
        return QStringLiteral("Fund");
    case AssetKind::Bond:
        return QStringLiteral("Bond");
    }
    return QStringLiteral("Unknown");
}

bool assetKindFromString(const QString& s, AssetKind* out)
{
    const QString t = s.trimmed();
    if (t.compare(QLatin1String("Equity"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Equity;
        return true;
    }
    if (t.compare(QLatin1String("Etf"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Etf;
        return true;
    }
    if (t.compare(QLatin1String("Forex"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Forex;
        return true;
    }
    if (t.compare(QLatin1String("Crypto"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Crypto;
        return true;
    }
    if (t.compare(QLatin1String("Future"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Future;
        return true;
    }
    if (t.compare(QLatin1String("Option"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Option;
        return true;
    }
    if (t.compare(QLatin1String("Index"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Index;
        return true;
    }
    if (t.compare(QLatin1String("Fund"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Fund;
        return true;
    }
    if (t.compare(QLatin1String("Bond"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Bond;
        return true;
    }
    if (t.compare(QLatin1String("Unknown"), Qt::CaseInsensitive) == 0) {
        *out = AssetKind::Unknown;
        return true;
    }
    return false;
}

} // namespace Backtest
