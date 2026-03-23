#include "Backtest/InstrumentMetadataIb.h"
#include "Backtest/InstrumentClassification.h"
#include "Backtest/InstrumentClassificationMappers.h"
#include "Backtest/InstrumentNormalization.h"
#include "DB/dbdatatypes.h"
#include "DB/dbquery.h"
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>

namespace Backtest {

void upsertInstrumentMetadataFromIbSecType(const QString& dbConnectionName,
                                           const QString& symbol,
                                           const QString& secType,
                                           const QString& exchange)
{
    if (dbConnectionName.isEmpty() || !QSqlDatabase::database(dbConnectionName).isOpen())
        return;

    const QString pid = QStringLiteral("ib");
    DbInstrumentMetadata row;
    row.providerSymbol = normalizeProviderSymbol(pid, symbol);
    row.providerId     = pid;
    row.assetKind      = assetKindToString(assetKindFromIbSecType(secType));
    row.sourceRawType  = secType;
    row.tradingScheduleId = exchange;
    row.updatedAt      = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QSqlQuery q = query_upsertInstrumentMetadata(row, dbConnectionName);
    q.exec();
}

} // namespace Backtest
