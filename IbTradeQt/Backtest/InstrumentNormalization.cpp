#include "Backtest/InstrumentNormalization.h"

namespace Backtest {

QString normalizeProviderSymbol(const QString& providerId, const QString& rawSymbol)
{
    QString s = rawSymbol.trimmed();
    if (providerId == QLatin1String("yahoo")) {
        // Yahoo chart URLs use encoded symbols; storage uses trimmed display form.
        return s;
    }
    if (providerId == QLatin1String("ib")) {
        return s.toUpper();
    }
    if (providerId == QLatin1String("csv")) {
        return s;
    }
    return s;
}

} // namespace Backtest
