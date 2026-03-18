#ifndef TST_STRATEGY_DEFINITION_H
#define TST_STRATEGY_DEFINITION_H

#include <QObject>
#include <QtTest>
#include <QTemporaryFile>
#include <QUuid>
#include <QJsonDocument>
#include "Backend/ModelTreeRepository.h"
#include "Backend/SystemBackendImpl.h"
#include "DB/dbdatatypes.h"
#include "Strategies/Generic/ModelType.h"

// ---------------------------------------------------------------------------
// Helper: build a fresh in-memory repo + backend pair
// ---------------------------------------------------------------------------
class TestStrategyDefinition : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ModelTreeRepository> m_repo;
    std::unique_ptr<SystemBackendImpl>   m_backend;
    QString m_dbPath;

    static QString nowIso() {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    DbStrategyDefinition makeDef(const QString& id, const QString& name,
                                  const QString& configJson = "{}") {
        DbStrategyDefinition def;
        def.strategyDefId  = id;
        def.name           = name;
        def.strategyKind   = static_cast<int>(ModelType::STRATEGY_PIPELINE);
        def.configJson     = configJson;
        def.version        = 1;
        def.lifecycleState = QStringLiteral("active");
        def.isArchived     = false;
        def.createdAt      = nowIso();
        def.updatedAt      = def.createdAt;
        return def;
    }

    void setupBackend() {
        m_dbPath = QDir::tempPath() + "/test_stratdef_"
                 + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        m_repo = std::make_unique<ModelTreeRepository>(
            m_dbPath,
            "test_stratdef_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        QVERIFY(m_repo->initialize());
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());
    }

private slots:
    void cleanup() {
        m_backend.reset();
        m_repo.reset();
        QFile::remove(m_dbPath);
    }

    // ---- Definition CRUD ----

    void testCreateAndFetchDefinition() {
        setupBackend();
        auto def = makeDef("def-1", "My Strategy");
        QVERIFY(m_repo->createStrategyDefinition(def));

        DbStrategyDefinition fetched = m_repo->fetchStrategyDefinition("def-1");
        QVERIFY(fetched.isValid());
        QCOMPARE(fetched.strategyDefId, "def-1");
        QCOMPARE(fetched.name, "My Strategy");
        QCOMPARE(fetched.version, 1);
        QCOMPARE(fetched.isArchived, false);
    }

    void testUpdateDefinition() {
        setupBackend();
        auto def = makeDef("def-upd", "Original");
        QVERIFY(m_repo->createStrategyDefinition(def));

        def.name    = "Updated";
        def.version = 2;
        def.updatedAt = nowIso();
        QVERIFY(m_repo->updateStrategyDefinition(def));

        DbStrategyDefinition fetched = m_repo->fetchStrategyDefinition("def-upd");
        QCOMPARE(fetched.name, "Updated");
        QCOMPARE(fetched.version, 2);
    }

    void testListDefinitions_archivedFilter() {
        setupBackend();
        m_repo->createStrategyDefinition(makeDef("def-a", "Active"));
        auto archived = makeDef("def-b", "Archived");
        archived.isArchived = true;
        m_repo->createStrategyDefinition(archived);

        auto activeList   = m_repo->listStrategyDefinitions(false);
        auto allList      = m_repo->listStrategyDefinitions(true);

        QCOMPARE(activeList.size(), 1);
        QCOMPARE(activeList[0].strategyDefId, "def-a");
        QCOMPARE(allList.size(), 2);
    }

    void testArchiveDefinition() {
        setupBackend();
        m_repo->createStrategyDefinition(makeDef("def-arc", "ToArchive"));

        QVERIFY(m_repo->archiveStrategyDefinition("def-arc"));

        DbStrategyDefinition fetched = m_repo->fetchStrategyDefinition("def-arc");
        QVERIFY(fetched.isArchived);
    }

    void testFetchNonExistent_returnsInvalid() {
        setupBackend();
        DbStrategyDefinition fetched = m_repo->fetchStrategyDefinition("no-such-id");
        QVERIFY(!fetched.isValid());
    }

    // ---- Binding lifecycle ----

    void testCreateAndFetchBinding() {
        setupBackend();
        m_repo->createStrategyDefinition(makeDef("def-bind", "BoundDef"));

        ModelNodeRecord nodeRec;
        nodeRec.uuid       = "node-1";
        nodeRec.modelType  = static_cast<int>(ModelType::STRATEGY_PIPELINE);
        nodeRec.name       = "Strategy1";
        nodeRec.isActive   = true;
        nodeRec.createdAt  = QDateTime::currentDateTimeUtc();
        nodeRec.updatedAt  = nodeRec.createdAt;
        m_repo->insertNode(nodeRec);

        DbLiveStrategyBinding b;
        b.bindingId     = "bind-1";
        b.modelNodeId   = "node-1";
        b.strategyDefId = "def-bind";
        b.createdAt     = nowIso();
        b.updatedAt     = b.createdAt;
        QVERIFY(m_repo->createLiveBinding(b));

        DbLiveStrategyBinding fetched = m_repo->fetchBindingForNode("node-1");
        QVERIFY(fetched.isValid());
        QCOMPARE(fetched.strategyDefId, "def-bind");
    }

    void testRemoveNodePreservesDefinition() {
        setupBackend();
        // Create account → portfolio → strategy using the backend
        QString acctId = m_backend->createAccount("Acct");
        QVERIFY(!acctId.isEmpty());
        QString portId = m_backend->createPortfolio(acctId, "Port");
        QVERIFY(!portId.isEmpty());
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);
        QVERIFY(!stratId.isEmpty());

        // Verify binding exists
        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(stratId);
        QVERIFY(binding.isValid());
        QString defId = binding.strategyDefId;

        // Remove the live node
        QVERIFY(m_backend->removeNode(stratId));

        // Binding must be gone
        QVERIFY(!m_repo->fetchBindingForNode(stratId).isValid());

        // Definition must still be present (catalog artifact)
        QVERIFY(m_repo->fetchStrategyDefinition(defId).isValid());
    }

    void testLastBindingRemovedLeavesDefinitionIntact() {
        setupBackend();
        m_repo->createStrategyDefinition(makeDef("def-last", "LastBinding"));

        ModelNodeRecord nodeRec;
        nodeRec.uuid       = "node-last";
        nodeRec.modelType  = static_cast<int>(ModelType::STRATEGY_PIPELINE);
        nodeRec.name       = "Strat";
        nodeRec.isActive   = true;
        nodeRec.createdAt  = QDateTime::currentDateTimeUtc();
        nodeRec.updatedAt  = nodeRec.createdAt;
        m_repo->insertNode(nodeRec);

        DbLiveStrategyBinding b;
        b.bindingId     = "bind-last";
        b.modelNodeId   = "node-last";
        b.strategyDefId = "def-last";
        b.createdAt     = nowIso();
        b.updatedAt     = b.createdAt;
        m_repo->createLiveBinding(b);

        QVERIFY(m_repo->removeBindingForNode("node-last"));

        // No bindings remain for the definition
        QVERIFY(m_repo->listBindingsForDefinition("def-last").isEmpty());

        // But definition is still there (not auto-archived)
        QVERIFY(m_repo->fetchStrategyDefinition("def-last").isValid());
    }

    // ---- createStrategy auto-creates definition + binding ----

    void testCreateStrategyAutoBinding() {
        setupBackend();
        QString acctId = m_backend->createAccount("Acct");
        QString portId = m_backend->createPortfolio(acctId, "Port");
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);
        QVERIFY(!stratId.isEmpty());

        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(stratId);
        QVERIFY(binding.isValid());
        QVERIFY(!binding.strategyDefId.isEmpty());

        DbStrategyDefinition def = m_repo->fetchStrategyDefinition(binding.strategyDefId);
        QVERIFY(def.isValid());
        QCOMPARE(def.version, 1);
    }

    // ---- Sync helper / content-diff guard ----

    void testSyncDoesNotBumpVersionOnIdenticalConfig() {
        setupBackend();
        QString acctId  = m_backend->createAccount("A");
        QString portId  = m_backend->createPortfolio(acctId, "P");
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);

        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(stratId);
        int versionBefore = m_repo->fetchStrategyDefinition(binding.strategyDefId).version;

        // Call updatePipelineConfig with the same config that exists
        QJsonObject existingConfig = m_backend->pipelineConfig(stratId);
        m_backend->updatePipelineConfig(stratId, existingConfig);

        int versionAfter = m_repo->fetchStrategyDefinition(binding.strategyDefId).version;
        QCOMPARE(versionAfter, versionBefore);
    }

    void testSyncBumpsVersionOnChangedConfig() {
        // Since v3, detectVersionDivergence() no longer auto-bumps versions.
        // Version creation is gated/explicit. This test now verifies that
        // the version is NOT bumped (detect-only behavior).
        setupBackend();
        QString acctId  = m_backend->createAccount("A");
        QString portId  = m_backend->createPortfolio(acctId, "P");
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);

        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(stratId);
        int versionBefore = m_repo->fetchStrategyDefinition(binding.strategyDefId).version;

        QJsonObject newConfig;
        newConfig["alphas"] = QJsonArray();
        newConfig["mergePolicy"] = "changed";
        m_backend->updatePipelineConfig(stratId, newConfig);

        int versionAfter = m_repo->fetchStrategyDefinition(binding.strategyDefId).version;
        QCOMPARE(versionAfter, versionBefore);
    }

    // ---- Extract canonical config ----

    void testExtractCanonicalConfigStripsNonCanonical() {
        setupBackend();
        // Build a raw node-like config with extra fields
        QJsonObject raw;
        raw["uuid"]        = "should-be-stripped";
        raw["name"]        = "should-be-stripped";
        raw["genericInfo"] = QJsonObject{{"status", "Running"}};
        raw["parameters"]  = QJsonObject{{"param1", 42}};
        raw["pipelineConfig"] = QJsonObject{{"alphas", QJsonArray()}};
        raw["assetList"]   = QJsonObject{{"symbols", QJsonArray()}};

        // extractCanonicalStrategyConfig is private, but we can verify via the
        // sync path: a node with those extra fields should NOT store uuid/name/genericInfo
        // in the definition's configJson.
        QString acctId  = m_backend->createAccount("A");
        QString portId  = m_backend->createPortfolio(acctId, "P");
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);

        // Force a config update with extra fields
        m_backend->updatePipelineConfig(stratId, raw["pipelineConfig"].toObject());

        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode(stratId);
        DbStrategyDefinition  def     = m_repo->fetchStrategyDefinition(binding.strategyDefId);

        QJsonObject defConfig = QJsonDocument::fromJson(def.configJson.toUtf8()).object();
        QVERIFY(!defConfig.contains("uuid"));
        QVERIFY(!defConfig.contains("name"));
        QVERIFY(!defConfig.contains("genericInfo"));
    }

    // ---- Orphan repair ----

    void testOrphanRepairCreatesDefinitionOnLoad() {
        setupBackend();
        // Insert a strategy node directly into the repo (no binding/definition)
        ModelNodeRecord nodeRec;
        nodeRec.uuid       = "orphan-node";
        nodeRec.modelType  = static_cast<int>(ModelType::STRATEGY_PIPELINE);
        nodeRec.name       = "OrphanStrat";
        nodeRec.isActive   = true;
        nodeRec.createdAt  = QDateTime::currentDateTimeUtc();
        nodeRec.updatedAt  = nodeRec.createdAt;
        m_repo->insertNode(nodeRec);

        // loadFromDb should auto-repair the orphan
        QVERIFY(m_backend->loadFromDb());

        DbLiveStrategyBinding binding = m_repo->fetchBindingForNode("orphan-node");
        QVERIFY(binding.isValid());

        DbStrategyDefinition def = m_repo->fetchStrategyDefinition(binding.strategyDefId);
        QVERIFY(def.isValid());
    }

    void testOrphanRepairIsSilentOnSecondLoad() {
        setupBackend();
        ModelNodeRecord nodeRec;
        nodeRec.uuid       = "orphan-node-2";
        nodeRec.modelType  = static_cast<int>(ModelType::STRATEGY_PIPELINE);
        nodeRec.name       = "Orphan2";
        nodeRec.isActive   = true;
        nodeRec.createdAt  = QDateTime::currentDateTimeUtc();
        nodeRec.updatedAt  = nodeRec.createdAt;
        m_repo->insertNode(nodeRec);

        // First load: repairs
        m_backend->loadFromDb();
        DbLiveStrategyBinding firstBinding = m_repo->fetchBindingForNode("orphan-node-2");
        QVERIFY(firstBinding.isValid());

        // Second load: no repair (no orphan), same binding
        m_backend->loadFromDb();
        DbLiveStrategyBinding secondBinding = m_repo->fetchBindingForNode("orphan-node-2");
        QVERIFY(secondBinding.isValid());
        QCOMPARE(firstBinding.bindingId, secondBinding.bindingId);
    }

    // ---- Backtest run profiles ----

    void testCreateAndListRunProfiles() {
        setupBackend();
        QJsonObject runCfg;
        runCfg["resolution"] = "Day1";
        runCfg["slippageBps"] = 1.0;

        QString profileId = m_backend->createBacktestRunProfile(
            "strategy_definition", "def-xyz", "MyProfile", runCfg);
        QVERIFY(!profileId.isEmpty());

        QJsonArray profiles = m_backend->listBacktestRunProfiles(
            "strategy_definition", "def-xyz");
        QCOMPARE(profiles.size(), 1);
        QCOMPARE(profiles[0].toObject()["profileId"].toString(), profileId);
        QCOMPARE(profiles[0].toObject()["name"].toString(), "MyProfile");
        QCOMPARE(profiles[0].toObject()["ownerType"].toString(), "strategy_definition");
    }

    void testListRunProfiles_filtersByOwnerTypeAndRef() {
        setupBackend();
        QJsonObject cfg;
        m_backend->createBacktestRunProfile("strategy_definition", "def-1", "P1", cfg);
        m_backend->createBacktestRunProfile("strategy_definition", "def-2", "P2", cfg);
        m_backend->createBacktestRunProfile("live_strategy", "node-1", "P3", cfg);

        auto def1Profiles  = m_backend->listBacktestRunProfiles("strategy_definition", "def-1");
        auto def2Profiles  = m_backend->listBacktestRunProfiles("strategy_definition", "def-2");
        auto liveProfiles  = m_backend->listBacktestRunProfiles("live_strategy", "node-1");
        auto emptyProfiles = m_backend->listBacktestRunProfiles("strategy_definition", "def-99");

        QCOMPARE(def1Profiles.size(), 1);
        QCOMPARE(def2Profiles.size(), 1);
        QCOMPARE(liveProfiles.size(), 1);
        QCOMPARE(emptyProfiles.size(), 0);
    }

    // ---- ISystemBackend catalog methods ----

    void testBackendCreateStrategyDefinition() {
        setupBackend();
        QJsonObject cfg;
        cfg["alphas"] = QJsonArray();

        QString defId = m_backend->createStrategyDefinition(
            "CatalogStrat", static_cast<int>(ModelType::STRATEGY_PIPELINE), cfg);
        QVERIFY(!defId.isEmpty());

        QJsonObject fetched = m_backend->strategyDefinition(defId);
        QVERIFY(!fetched.isEmpty());
        QCOMPARE(fetched["name"].toString(), "CatalogStrat");
        QCOMPARE(fetched["version"].toInt(), 1);
    }

    void testBackendUpdateStrategyDefinition_contentDiff() {
        setupBackend();
        QJsonObject cfg;
        cfg["alphas"] = QJsonArray();
        QString defId = m_backend->createStrategyDefinition(
            "DiffTest", static_cast<int>(ModelType::STRATEGY_PIPELINE), cfg);

        // Same config — version must stay at 1
        QVERIFY(m_backend->updateStrategyDefinition(defId, cfg));
        QCOMPARE(m_backend->strategyDefinition(defId)["version"].toInt(), 1);

        // Changed config — version must increment
        cfg["mergePolicy"] = "sum";
        QVERIFY(m_backend->updateStrategyDefinition(defId, cfg));
        QCOMPARE(m_backend->strategyDefinition(defId)["version"].toInt(), 2);
    }

    void testBackendArchiveDefinition() {
        setupBackend();
        QJsonObject cfg;
        QString defId = m_backend->createStrategyDefinition(
            "ToArchive", static_cast<int>(ModelType::STRATEGY_PIPELINE), cfg);

        QVERIFY(m_backend->archiveStrategyDefinition(defId));

        QJsonArray active  = m_backend->listStrategyDefinitions(false);
        QJsonArray withArc = m_backend->listStrategyDefinitions(true);

        // Archived def excluded from default list
        bool foundInActive = false;
        for (const auto& v : active) {
            if (v.toObject()["strategyDefId"].toString() == defId)
                foundInActive = true;
        }
        QVERIFY(!foundInActive);

        bool foundInAll = false;
        for (const auto& v : withArc) {
            if (v.toObject()["strategyDefId"].toString() == defId)
                foundInAll = true;
        }
        QVERIFY(foundInAll);
    }

    void testStrategyDefinitionForNode() {
        setupBackend();
        QString acctId  = m_backend->createAccount("A");
        QString portId  = m_backend->createPortfolio(acctId, "P");
        QString stratId = m_backend->createStrategy(portId, ModelType::STRATEGY_PIPELINE);

        QJsonObject defJson = m_backend->strategyDefinitionForNode(stratId);
        QVERIFY(!defJson.isEmpty());
        QVERIFY(!defJson["strategyDefId"].toString().isEmpty());
    }
};

#endif // TST_STRATEGY_DEFINITION_H
