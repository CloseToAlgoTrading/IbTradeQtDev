#ifndef BACKTEST_INSTRUMENTNORMALIZATION_H
#define BACKTEST_INSTRUMENTNORMALIZATION_H

#include <QString>

namespace Backtest {

/// Normalize lookup key for a provider before DB PK use.
QString normalizeProviderSymbol(const QString& providerId, const QString& rawSymbol);

} // namespace Backtest

#endif
