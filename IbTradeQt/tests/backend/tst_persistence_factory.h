#ifndef TST_PERSISTENCE_FACTORY_H
#define TST_PERSISTENCE_FACTORY_H

#include <QObject>
#include <QtTest>
#include <QDir>
#include <QUuid>
#include "Backend/IModelTreeRepository.h"
#include "Backend/PersistenceFactory.h"
#include "Backend/ModelTreeRepository.h"
#include "Common/StorageConfig.h"

class TestPersistenceFactory : public QObject
{
    Q_OBJECT

private slots:
    void factoryCreatesSqliteRepoInitializes()
    {
        const QString dbPath = QDir::tempPath() + "/test_persist_factory_"
            + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        QFile::remove(dbPath);

        StorageConfig cfg = StorageConfig::loadDefaults();
        cfg.modelStore.backend = StorageBackend::Sqlite;
        cfg.modelStore.sqlitePath = dbPath;

        auto repo = Persistence::createModelTreeRepository(cfg, QStringLiteral("factory_test_conn"));
        QVERIFY(repo);
        auto* concrete = dynamic_cast<ModelTreeRepository*>(repo.get());
        QVERIFY(concrete);
        QVERIFY(repo->initialize());
        QCOMPARE(repo->metadata(QStringLiteral("schema_version")), QStringLiteral("3"));

        repo.reset();
        QFile::remove(dbPath);
    }

    void factorySelectsPostgresWhenConfigured()
    {
        StorageConfig cfg = StorageConfig::loadDefaults();
        cfg.modelStore.backend = StorageBackend::Postgresql;

        auto repo = Persistence::createModelTreeRepository(cfg, QStringLiteral("factory_pg_conn"));
        QVERIFY(repo);
        QVERIFY(dynamic_cast<ModelTreeRepository*>(repo.get()) == nullptr);
    }
};

#endif
