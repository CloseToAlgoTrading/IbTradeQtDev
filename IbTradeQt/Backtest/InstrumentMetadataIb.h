#ifndef BACKTEST_INSTRUMENTMETADATAIB_H
#define BACKTEST_INSTRUMENTMETADATAIB_H

#include <QString>

namespace Backtest {

/// Upsert provider-scoped InstrumentMetadata for IB from contract secType (mutable snapshot).
/// \a exchange optional venue hint; stored in \c tradingScheduleId for v1 traceability.
void upsertInstrumentMetadataFromIbSecType(const QString& dbConnectionName,
                                         const QString& symbol,
                                         const QString& secType,
                                         const QString& exchange = {});

} // namespace Backtest

#endif
