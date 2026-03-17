#ifndef TST_MODEL_TREE_REPOSITORY_H
#define TST_MODEL_TREE_REPOSITORY_H

#include <QObject>
#include <QtTest>
#include <QTemporaryFile>
#include <QJsonArray>
#include "Backend/ModelTreeRepository.h"
#include "Backend/ModelTreeMapper.h"
#include "Strategies/Generic/cbasicroot.h"
#include "Strategies/Generic/cbasicaccount.h"
#include "Strategies/Generic/cbasicportfolio.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"
#include "Strategies/Generic/ModelType.h"

class TestModelTreeRepository : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ModelTreeRepository> m_repo;
    QString m_dbPath;

    ModelNodeRecord makeRecord(const QString& uuid,
                               const QString& parentUuid,
                               int modelType,
                               const QString& name,
                               int sortOrder = 0)
    {
        ModelNodeRecord rec;
        rec.uuid = uuid;
        rec.parentUuid = parentUuid;
        rec.modelType = modelType;
        rec.name = name;
        rec.sortOrder = sortOrder;
        rec.isActive = true;
        rec.createdAt = QDateTime::currentDateTimeUtc();
        rec.updatedAt = rec.createdAt;
        return rec;
    }

private slots:
    void init()
    {
        m_dbPath = QDir::tempPath() + "/test_model_tree_" +
                   QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        m_repo = std::make_unique<ModelTreeRepository>(m_dbPath, "test_conn_" +
                   QUuid::createUuid().toString(QUuid::WithoutBraces));
        QVERIFY(m_repo->initialize());
    }

    void cleanup()
    {
        m_repo.reset();
        QFile::remove(m_dbPath);
    }

    void testCreateTablesOnInit()
    {
        QVERIFY(m_repo->initialize());
    }

    void testInsertAndFetch()
    {
        auto rec = makeRecord("aaa-111", "", static_cast<int>(ModelType::ACCOUNT), "TestAccount");
        rec.config = QJsonObject{{"key", "value"}};
        QVERIFY(m_repo->insertNode(rec));

        auto fetched = m_repo->fetchNode("aaa-111");
        QVERIFY(fetched.has_value());
        QCOMPARE(fetched->uuid, "aaa-111");
        QCOMPARE(fetched->parentUuid, "");
        QCOMPARE(fetched->modelType, static_cast<int>(ModelType::ACCOUNT));
        QCOMPARE(fetched->name, "TestAccount");
        QCOMPARE(fetched->config["key"].toString(), "value");
        QCOMPARE(fetched->isActive, true);
    }

    void testFetchChildren()
    {
        auto parent = makeRecord("parent-1", "", static_cast<int>(ModelType::ACCOUNT), "Parent", 0);
        QVERIFY(m_repo->insertNode(parent));

        for (int i = 0; i < 3; ++i) {
            auto child = makeRecord(
                QString("child-%1").arg(i), "parent-1",
                static_cast<int>(ModelType::PORTFOLIO),
                QString("Child%1").arg(i), i);
            QVERIFY(m_repo->insertNode(child));
        }

        auto children = m_repo->fetchChildren("parent-1");
        QCOMPARE(children.size(), 3);
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(children[i].sortOrder, i);
            QCOMPARE(children[i].name, QString("Child%1").arg(i));
        }
    }

    void testFetchTopLevel()
    {
        QVERIFY(m_repo->insertNode(
            makeRecord("acc-1", "", static_cast<int>(ModelType::ACCOUNT), "Acct1", 0)));
        QVERIFY(m_repo->insertNode(
            makeRecord("acc-2", "", static_cast<int>(ModelType::ACCOUNT), "Acct2", 1)));

        auto topLevel = m_repo->fetchTopLevel();
        QCOMPARE(topLevel.size(), 2);
    }

    void testDeleteCascade()
    {
        auto acc = makeRecord("acc-del", "", static_cast<int>(ModelType::ACCOUNT), "Account");
        QVERIFY(m_repo->insertNode(acc));

        auto port = makeRecord("port-del", "acc-del", static_cast<int>(ModelType::PORTFOLIO), "Portfolio");
        QVERIFY(m_repo->insertNode(port));

        auto strat = makeRecord("strat-del", "port-del", static_cast<int>(ModelType::STRATEGY_PIPELINE), "Strategy");
        QVERIFY(m_repo->insertNode(strat));

        QVERIFY(m_repo->deleteNode("acc-del"));

        QVERIFY(!m_repo->fetchNode("acc-del").has_value());
        QVERIFY(!m_repo->fetchNode("port-del").has_value());
        QVERIFY(!m_repo->fetchNode("strat-del").has_value());
    }

    void testReplaceAll()
    {
        QVERIFY(m_repo->insertNode(
            makeRecord("old-1", "", static_cast<int>(ModelType::ACCOUNT), "Old")));

        QList<ModelNodeRecord> newRecords;
        newRecords.append(makeRecord("new-1", "", static_cast<int>(ModelType::ACCOUNT), "New1", 0));
        newRecords.append(makeRecord("new-2", "", static_cast<int>(ModelType::ACCOUNT), "New2", 1));

        QVERIFY(m_repo->replaceAll(newRecords));

        QVERIFY(!m_repo->fetchNode("old-1").has_value());
        QVERIFY(m_repo->fetchNode("new-1").has_value());
        QVERIFY(m_repo->fetchNode("new-2").has_value());
        QCOMPARE(m_repo->fetchAll().size(), 2);
    }

    void testNextSortOrder()
    {
        QCOMPARE(m_repo->nextSortOrder(""), 0);

        QVERIFY(m_repo->insertNode(
            makeRecord("s1", "", static_cast<int>(ModelType::ACCOUNT), "A", 0)));
        QVERIFY(m_repo->insertNode(
            makeRecord("s2", "", static_cast<int>(ModelType::ACCOUNT), "B", 1)));
        QVERIFY(m_repo->insertNode(
            makeRecord("s3", "", static_cast<int>(ModelType::ACCOUNT), "C", 2)));

        QCOMPARE(m_repo->nextSortOrder(""), 3);
    }

    void testMetadata()
    {
        QVERIFY(m_repo->setMetadata("schema_version", "1"));
        QCOMPARE(m_repo->metadata("schema_version"), "1");

        QVERIFY(m_repo->setMetadata("schema_version", "2"));
        QCOMPARE(m_repo->metadata("schema_version"), "2");

        QCOMPARE(m_repo->metadata("nonexistent"), "");
    }

    void testMapperToRecords()
    {
        CBasicRoot root;
        root.setId(QUuid::createUuid());

        auto account = QSharedPointer<CBasicAccount>::create();
        account->setId(QUuid::createUuid());
        account->setName("TestAccount");
        account->setParentModel(&root);
        root.getModels().append(account);

        auto portfolio = QSharedPointer<CBasicPortfolio>::create();
        portfolio->setId(QUuid::createUuid());
        portfolio->setName("TestPortfolio");
        portfolio->setParentModel(account.data());
        account->getModels().append(portfolio);

        auto strategy = QSharedPointer<CPipelineStrategyAdapter>::create();
        strategy->setId(QUuid::createUuid());
        strategy->setName("TestStrategy");
        strategy->setParentModel(portfolio.data());
        portfolio->getModels().append(strategy);

        auto records = ModelTreeMapper::toRecords(&root);

        QCOMPARE(records.size(), 3);

        bool hasRoot = false;
        for (const auto& r : records) {
            if (r.modelType == static_cast<int>(ModelType::ROOT))
                hasRoot = true;
        }
        QVERIFY2(!hasRoot, "ROOT must not be in records");

        auto accRec = records[0];
        QCOMPARE(accRec.modelType, static_cast<int>(ModelType::ACCOUNT));
        QVERIFY(accRec.parentUuid.isEmpty());

        auto portRec = records[1];
        QCOMPARE(portRec.modelType, static_cast<int>(ModelType::PORTFOLIO));
        QCOMPARE(portRec.parentUuid, accRec.uuid);

        auto stratRec = records[2];
        QCOMPARE(stratRec.modelType, static_cast<int>(ModelType::STRATEGY_PIPELINE));
        QCOMPARE(stratRec.parentUuid, portRec.uuid);

        QVERIFY(!stratRec.config.contains("genericInfo"));
    }

    void testMapperToRoot()
    {
        QList<ModelNodeRecord> records;

        auto accRec = makeRecord("acc-uuid", "", static_cast<int>(ModelType::ACCOUNT), "MyAccount", 0);
        accRec.config = QJsonObject{
            {"m_uuid", "acc-uuid"},
            {"modelType", static_cast<int>(ModelType::ACCOUNT)},
            {"parameters", QJsonObject{{"Name", "MyAccount"}, {"Description", ""}}},
            {"info", QJsonObject()},
            {"assetList", QJsonObject()},
        };
        records.append(accRec);

        auto portRec = makeRecord("port-uuid", "acc-uuid", static_cast<int>(ModelType::PORTFOLIO), "MyPortfolio", 0);
        portRec.config = QJsonObject{
            {"m_uuid", "port-uuid"},
            {"modelType", static_cast<int>(ModelType::PORTFOLIO)},
            {"parameters", QJsonObject{{"Name", "MyPortfolio"}, {"Description", ""}}},
            {"info", QJsonObject()},
            {"assetList", QJsonObject()},
        };
        records.append(portRec);

        CBasicRoot* root = ModelTreeMapper::toRoot(records);
        QVERIFY(root != nullptr);

        auto& accounts = root->getModels();
        QCOMPARE(accounts.size(), 1);
        QCOMPARE(accounts[0]->getName(), "MyAccount");
        QCOMPARE(accounts[0]->modelType(), ModelType::ACCOUNT);

        auto& portfolios = accounts[0]->getModels();
        QCOMPARE(portfolios.size(), 1);
        QCOMPARE(portfolios[0]->getName(), "MyPortfolio");
        QCOMPARE(portfolios[0]->modelType(), ModelType::PORTFOLIO);

        delete root;
    }

    void testMapperRoundtrip()
    {
        CBasicRoot root;
        root.setId(QUuid::createUuid());

        auto account = QSharedPointer<CBasicAccount>::create();
        account->setId(QUuid::createUuid());
        account->setName("RoundtripAccount");
        account->setParentModel(&root);
        root.getModels().append(account);

        auto portfolio = QSharedPointer<CBasicPortfolio>::create();
        portfolio->setId(QUuid::createUuid());
        portfolio->setName("RoundtripPortfolio");
        portfolio->setParentModel(account.data());
        account->getModels().append(portfolio);

        auto records1 = ModelTreeMapper::toRecords(&root);
        QCOMPARE(records1.size(), 2);

        CBasicRoot* rebuilt = ModelTreeMapper::toRoot(records1);
        QVERIFY(rebuilt != nullptr);

        auto records2 = ModelTreeMapper::toRecords(rebuilt);
        QCOMPARE(records2.size(), 2);

        for (int i = 0; i < records1.size(); ++i) {
            QCOMPARE(records1[i].uuid, records2[i].uuid);
            QCOMPARE(records1[i].parentUuid, records2[i].parentUuid);
            QCOMPARE(records1[i].modelType, records2[i].modelType);
            QCOMPARE(records1[i].name, records2[i].name);
        }

        delete rebuilt;
    }

    void testPipelineConfigPreserved()
    {
        CBasicRoot root;
        root.setId(QUuid::createUuid());

        auto account = QSharedPointer<CBasicAccount>::create();
        account->setId(QUuid::createUuid());
        account->setName("Acc");
        account->setParentModel(&root);
        root.getModels().append(account);

        auto portfolio = QSharedPointer<CBasicPortfolio>::create();
        portfolio->setId(QUuid::createUuid());
        portfolio->setName("Port");
        portfolio->setParentModel(account.data());
        account->getModels().append(portfolio);

        auto strategy = QSharedPointer<CPipelineStrategyAdapter>::create();
        strategy->setId(QUuid::createUuid());
        strategy->setName("PipeStrat");
        strategy->setParentModel(portfolio.data());

        QJsonObject pipeCfg;
        QJsonArray alphas;
        QJsonObject alpha;
        alpha["blockId"] = "MomentumAlpha";
        alpha["config"] = QJsonObject{{"lookback", 14}};
        alphas.append(alpha);
        pipeCfg["alphas"] = alphas;
        pipeCfg["mergePolicy"] = "sum";
        strategy->setPipelineConfig(pipeCfg);

        portfolio->getModels().append(strategy);

        auto records = ModelTreeMapper::toRecords(&root);
        CBasicRoot* rebuilt = ModelTreeMapper::toRoot(records);
        QVERIFY(rebuilt != nullptr);

        auto& accts = rebuilt->getModels();
        QCOMPARE(accts.size(), 1);
        auto& ports = accts[0]->getModels();
        QCOMPARE(ports.size(), 1);
        auto& strats = ports[0]->getModels();
        QCOMPARE(strats.size(), 1);

        auto* adapter = dynamic_cast<CPipelineStrategyAdapter*>(strats[0].data());
        QVERIFY(adapter != nullptr);

        const QJsonObject& restoredCfg = adapter->pipelineConfig();
        QCOMPARE(restoredCfg["mergePolicy"].toString(), "sum");
        QCOMPARE(restoredCfg["alphas"].toArray().size(), 1);
        QCOMPARE(restoredCfg["alphas"].toArray()[0].toObject()["blockId"].toString(), "MomentumAlpha");

        delete rebuilt;
    }

    void testRootNotInDb()
    {
        CBasicRoot root;
        root.setId(QUuid::createUuid());

        auto account = QSharedPointer<CBasicAccount>::create();
        account->setId(QUuid::createUuid());
        account->setName("Acc");
        account->setParentModel(&root);
        root.getModels().append(account);

        auto records = ModelTreeMapper::toRecords(&root);
        for (const auto& r : records) {
            QVERIFY2(r.modelType != static_cast<int>(ModelType::ROOT),
                     "ROOT must never appear in persisted records");
        }
    }

    void testUpdateNode()
    {
        auto rec = makeRecord("upd-1", "", static_cast<int>(ModelType::ACCOUNT), "Before");
        QVERIFY(m_repo->insertNode(rec));

        rec.name = "After";
        rec.updatedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(m_repo->updateNode(rec));

        auto fetched = m_repo->fetchNode("upd-1");
        QVERIFY(fetched.has_value());
        QCOMPARE(fetched->name, "After");
    }

    void testFetchAll()
    {
        QVERIFY(m_repo->insertNode(
            makeRecord("fa-1", "", static_cast<int>(ModelType::ACCOUNT), "A1", 0)));
        QVERIFY(m_repo->insertNode(
            makeRecord("fa-2", "fa-1", static_cast<int>(ModelType::PORTFOLIO), "P1", 0)));

        auto all = m_repo->fetchAll();
        QCOMPARE(all.size(), 2);
    }
};

#endif // TST_MODEL_TREE_REPOSITORY_H
