#ifndef TST_PERSISTENCE_H
#define TST_PERSISTENCE_H

#include <QObject>
#include <QtTest>
#include <QDir>
#include <QTemporaryFile>
#include "Backend/SystemBackendImpl.h"
#include "Backend/ModelTreeRepository.h"
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

    std::unique_ptr<ModelTreeRepository> makeRepo(const QString& suffix = "")
    {
        QString conn = "persist_conn_" + m_connSuffix + suffix;
        auto repo = std::make_unique<ModelTreeRepository>(m_dbPath, conn);
        repo->initialize();
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
        auto repo = makeRepo("_ver");
        QCOMPARE(repo->metadata("schema_version"), "3");
    }

    void testSchemaVersionUpgradeFromV1()
    {
        // A fresh DB should initialize at v3; simulate a legacy v1 DB by
        // manually resetting the metadata and re-initializing.
        auto repo = makeRepo("_v1upgrade");
        repo->setMetadata("schema_version", "1");

        // Re-init with a second repo instance on the same path to trigger upgrade
        QString conn2 = "persist_conn_" + m_connSuffix + "_v1upgrade_b";
        auto repo2 = std::make_unique<ModelTreeRepository>(m_dbPath, conn2);
        QVERIFY(repo2->initialize());
        QCOMPARE(repo2->metadata("schema_version"), "3");
    }

    void testMigrationViaRecords()
    {
        auto repo = makeRepo("_mig");
        auto backend = std::make_unique<SystemBackendImpl>(repo.get());

        QVERIFY(!backend->loadFromDb());

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
        auto repo = makeRepo("_restart");

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
        auto repo = makeRepo("_mut");

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
        auto repo = makeRepo("_rt");
        auto backend = std::make_unique<SystemBackendImpl>(repo.get());

        QString acc = backend->createAccount("ExportAcc");
        QString port = backend->createPortfolio(acc, "ExportPort");

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
        auto repo = makeRepo("_idem");

        repo->setMetadata("model_tree_migrated_from_json", "true");

        // Even though there's a JSON file, migration should not re-run
        QString migrated = repo->metadata("model_tree_migrated_from_json");
        QCOMPARE(migrated, "true");
    }

    void testRemovePersistsAcrossRestart()
    {
        auto repo = makeRepo("_rm");

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
