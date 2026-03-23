#include "tst_instrument_metadata_resolver.h"

#include "Backtest/InstrumentClassification.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include "DB/dbdatatypes.h"
#include "DB/dbquery.h"
#include "Strategies/Generic/mandatoryFieldKeys.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QUuid>
#include <QtTest>

using namespace Backtest;

void TestInstrumentMetadataResolver::resolve_classificationOverrideWins()
{
    QVariantMap entry;
    entry[AssetFields::Position::ClassificationOverride] = QStringLiteral("Etf");

    const EffectiveInstrumentProfile p =
        InstrumentMetadataResolver::resolve(QString(), QStringLiteral("SPY"), QStringLiteral("yahoo"), entry);

    QCOMPARE(p.effectiveAssetKind, AssetKind::Etf);
    QCOMPARE(p.provenance, InstrumentClassificationProvenance::ClassificationOverride);
    QVERIFY(!p.isInferred);
}

void TestInstrumentMetadataResolver::resolve_infersWhenNoRow()
{
    const EffectiveInstrumentProfile p =
        InstrumentMetadataResolver::resolve(QString(), QStringLiteral("SPY"), QStringLiteral("yahoo"), {});

    QCOMPARE(p.effectiveAssetKind, AssetKind::Equity);
    QCOMPARE(p.provenance, InstrumentClassificationProvenance::InferredHeuristic);
    QVERIFY(p.isInferred);
}

void TestInstrumentMetadataResolver::resolve_usesProviderMetadataRow()
{
    const QString conn =
        QStringLiteral("imr_prov_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    QTemporaryFile dbFile;
    dbFile.setAutoRemove(true);
    QVERIFY(dbFile.open());
    dbFile.close();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbFile.fileName());
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec(QLatin1String(CREATE_TABLE_INSTRUMENT_METADATA)));
        DbInstrumentMetadata row;
        row.providerSymbol = QStringLiteral("SPY");
        row.providerId     = QStringLiteral("yahoo");
        row.assetKind        = QStringLiteral("Future");
        row.sourceRawType    = QStringLiteral("FUT");
        row.updatedAt        = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        QVERIFY(query_upsertInstrumentMetadata(row, conn).exec());
    }

    const EffectiveInstrumentProfile p =
        InstrumentMetadataResolver::resolve(conn, QStringLiteral("SPY"), QStringLiteral("yahoo"), {});

    QCOMPARE(p.effectiveAssetKind, AssetKind::Future);
    QCOMPARE(p.provenance, InstrumentClassificationProvenance::ProviderMetadata);
    QVERIFY(!p.isInferred);

    {
        QSqlDatabase db = QSqlDatabase::database(conn);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

void TestInstrumentMetadataResolver::resolve_sessionPolicyPassesThrough()
{
    QVariantMap entry;
    entry[AssetFields::Position::SessionPolicy] = QStringLiteral("usEquityLike");

    const EffectiveInstrumentProfile p =
        InstrumentMetadataResolver::resolve(QString(), QStringLiteral("SPY"), QStringLiteral("yahoo"), entry);

    QVERIFY(p.sessionPolicy.has_value());
    QCOMPARE(p.sessionPolicy.value(), QStringLiteral("usEquityLike"));
}
