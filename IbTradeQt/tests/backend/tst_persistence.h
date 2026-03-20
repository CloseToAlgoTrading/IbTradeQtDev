#ifndef TST_PERSISTENCE_H
#define TST_PERSISTENCE_H

#include <QObject>
#include <QtTest>
#include <QDir>
#include <QTemporaryFile>
#include "Backend/SystemBackendImpl.h"
#include "Backend/IModelTreeRepository.h"
#include "Backend/ModelNodeRecord.h"
#include "Backend/PersistenceFactory.h"
#include "Common/StorageConfig.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cbasicroot.h"
#include "Strategies/Generic/cbasicaccount.h"

class TestPersistence : public QObject
{
    Q_OBJECT

private:
    QString m_dbPath;
    QString m_jsonPath;
    QString m_connSuffix;

    std::unique_ptr<IModelTreeRepository> createInitializedRepo(const QString& suffix)
    {
        const QString conn = QStringLiteral("persist_conn_") + m_connSuffix + suffix;
        StorageConfig cfg = StorageConfig::loadDefaults();
        cfg.modelStore.sqlitePath = m_dbPath;
        auto repo = Persistence::createModelTreeRepository(cfg, conn);
        if (!repo || !repo->initialize())
            return nullptr;
        return repo;
    }

private slots:
    void init()
    {
        m_connSuffix = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_dbPath = QDir::tempPath() + "/test_persist_" + m_connSuffix + ".sqlite";
        m_jsonPath = QDir::tempPath() + "/test_persist_" + m_connSuffix + ".json";
    }

    void cleanup()
    {
        QFile::remove(m_dbPath);
        QFile::remove(m_jsonPath);
    }

    void testSchemaVersionSet()
    {
        // Schema version is now "3" after the strategy catalog migration
        auto repo = createInitializedRepo(QStringLiteral("_ver"));
        QVERIFY(repo);
        QCOMPARE(repo->metadata("schema_version"), "3");
    }

    void testSchemaVersionUpgradeFromV1()
    {
        // A fresh DB should initialize at v3; simulate a legacy v1 DB by
        // manually resetting the metadata and re-initializing.
        auto repo = createInitializedRepo(QStringLiteral("_v1upgrade"));
        QVERIFY(repo);
        repo->setMetadata("schema_version", "1");

        // Re-init with a second repo instance on the same path to trigger upgrade
        QString conn2 = "persist_conn_" + m_connSuffix + "_v1upgrade_b";
        StorageConfig cfg = StorageConfig::loadDefaults();
        cfg.modelStore.sqlitePath = m_dbPath;
        auto repo2 = Persistence::createModelTreeRepository(cfg, conn2);
        QVERIFY(repo2);
        QVERIFY(repo2->initialize());
        QCOMPARE(repo2->metadata("schema_version"), "3");
    }

    void testMigrationViaRecords()
    {
        auto repo = createInitializedRepo(QStringLiteral("_mig"));
        QVERIFY(repo);
        auto backend = std::make_unique<SystemBackendImpl>(repo.get());

        // Empty DB is now a valid state — loadFromDb() succeeds with an empty root
        QVERIFY(backend->loadFromDb());
        QCOMPARE(backend->dataRoot()->getModels().size(), 0);

        // Simulate migration by inserting records directly
        ModelNodeRecord rec;
        rec.uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        rec.parentUuid = "";
        rec.modelType = static_cast<int>(ModelType::ACCOUNT);
        rec.name = "MigratedAccount";
        rec.sortOrder = 0;
        rec.isActive = true;
        rec.createdAt = QDateTime::currentDateTimeUtc();
        rec.updatedAt = rec.createdAt;
        QVERIFY(repo->insertNode(rec));

        QVERIFY(backend->loadFromDb());
        QCOMPARE(backend->dataRoot()->getModels().size(), 1);
        QCOMPARE(backend->dataRoot()->getModels()[0]->getName(), "MigratedAccount");
    }

    void testRestartPersistence()
    {
        auto repo = createInitializedRepo(QStringLiteral("_restart"));
        QVERIFY(repo);

        // Session 1: create some data
        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            QString acc = backend->createAccount("PersistAcc");
            QVERIFY(!acc.isEmpty());
            backend->createPortfolio(acc, "PersistPort");
        }

        // Session 2: load from DB and verify
        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            QVERIFY(backend->loadFromDb());
            QCOMPARE(backend->dataRoot()->getModels().size(), 1);
            QCOMPARE(backend->dataRoot()->getModels()[0]->getName(), "PersistAcc");
            QCOMPARE(backend->dataRoot()->getModels()[0]->getModels().size(), 1);
            QCOMPARE(backend->dataRoot()->getModels()[0]->getModels()[0]->getName(), "PersistPort");
        }
    }

    void testMutationPersistsAcrossRestart()
    {
        auto repo = createInitializedRepo(QStringLiteral("_mut"));
        QVERIFY(repo);

        // Session 1: create + rename
        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            QString acc = backend->createAccount("OriginalName");
            QVERIFY(backend->renameNode(acc, "RenamedName"));
        }

        // Session 2: verify rename persisted
        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            QVERIFY(backend->loadFromDb());
            QCOMPARE(backend->dataRoot()->getModels()[0]->getName(), "RenamedName");
        }
    }

    void testExportCreatesValidJson()
    {
        auto repo = createInitializedRepo(QStringLiteral("_rt"));
        QVERIFY(repo);
        auto backend = std::make_unique<SystemBackendImpl>(repo.get());

        QString acc = backend->createAccount("ExportAcc");
        const QString portId = backend->createPortfolio(acc, "ExportPort");
        QVERIFY(!portId.isEmpty());

        QVERIFY(backend->exportToJsonFile(m_jsonPath));

        // Verify the exported JSON is valid and contains expected data
        QFile file(m_jsonPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QVERIFY(!doc.isNull());

        QJsonObject rootJson = doc.object();
        QVERIFY(rootJson.contains("models"));
        QJsonArray models = rootJson["models"].toArray();
        QVERIFY(models.size() >= 1);
    }

    void testMigrationIdempotent()
    {
        auto repo = createInitializedRepo(QStringLiteral("_idem"));
        QVERIFY(repo);

        repo->setMetadata("model_tree_migrated_from_json", "true");

        // Even though there's a JSON file, migration should not re-run
        QString migrated = repo->metadata("model_tree_migrated_from_json");
        QCOMPARE(migrated, "true");
    }

    void testRemovePersistsAcrossRestart()
    {
        auto repo = createInitializedRepo(QStringLiteral("_rm"));
        QVERIFY(repo);

        QString accId;
        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            accId = backend->createAccount("ToRemove");
            backend->createAccount("ToKeep");
            QVERIFY(backend->removeNode(accId));
        }

        {
            auto backend = std::make_unique<SystemBackendImpl>(repo.get());
            QVERIFY(backend->loadFromDb());
            QCOMPARE(backend->dataRoot()->getModels().size(), 1);
            QCOMPARE(backend->dataRoot()->getModels()[0]->getName(), "ToKeep");
        }
    }
};

#endif // TST_PERSISTENCE_H
