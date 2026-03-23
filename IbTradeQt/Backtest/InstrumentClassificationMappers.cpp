#include "Backtest/InstrumentClassificationMappers.h"

namespace Backtest {

AssetKind assetKindFromYahooInstrumentType(const QString& instrumentType, const QString& quoteType)
{
    const QString it = instrumentType.trimmed();
    const QString qt = quoteType.trimmed();

    if (it.compare(QLatin1String("EQUITY"), Qt::CaseInsensitive) == 0)
        return AssetKind::Equity;
    if (it.compare(QLatin1String("ETF"), Qt::CaseInsensitive) == 0)
        return AssetKind::Etf;
    if (it.compare(QLatin1String("INDEX"), Qt::CaseInsensitive) == 0)
        return AssetKind::Index;
    if (it.compare(QLatin1String("MUTUALFUND"), Qt::CaseInsensitive) == 0)
        return AssetKind::Fund;
    if (it.compare(QLatin1String("CURRENCY"), Qt::CaseInsensitive) == 0)
        return AssetKind::Forex;
    if (it.compare(QLatin1String("CRYPTOCURRENCY"), Qt::CaseInsensitive) == 0)
        return AssetKind::Crypto;
    if (it.compare(QLatin1String("FUTURE"), Qt::CaseInsensitive) == 0)
        return AssetKind::Future;
    if (it.compare(QLatin1String("OPTION"), Qt::CaseInsensitive) == 0)
        return AssetKind::Option;

    if (qt.compare(QLatin1String("EQUITY"), Qt::CaseInsensitive) == 0)
        return AssetKind::Equity;
    if (qt.compare(QLatin1String("ETF"), Qt::CaseInsensitive) == 0)
        return AssetKind::Etf;

    return AssetKind::Unknown;
}

AssetKind assetKindFromIbSecType(const QString& secType)
{
    const QString s = secType.trimmed();
    if (s.compare(QLatin1String("STK"), Qt::CaseInsensitive) == 0)
        return AssetKind::Equity;
    if (s.compare(QLatin1String("OPT"), Qt::CaseInsensitive) == 0)
        return AssetKind::Option;
    if (s.compare(QLatin1String("FUT"), Qt::CaseInsensitive) == 0)
        return AssetKind::Future;
    if (s.compare(QLatin1String("CASH"), Qt::CaseInsensitive) == 0)
        return AssetKind::Forex;
    if (s.compare(QLatin1String("CRYPTO"), Qt::CaseInsensitive) == 0)
        return AssetKind::Crypto;
    if (s.compare(QLatin1String("BOND"), Qt::CaseInsensitive) == 0)
        return AssetKind::Bond;
    if (s.compare(QLatin1String("FUND"), Qt::CaseInsensitive) == 0)
        return AssetKind::Fund;
    return AssetKind::Unknown;
}

} // namespace Backtest
