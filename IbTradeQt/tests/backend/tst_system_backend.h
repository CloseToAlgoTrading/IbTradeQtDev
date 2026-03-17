#ifndef TST_SYSTEM_BACKEND_H
#define TST_SYSTEM_BACKEND_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>
#include "Backend/SystemBackendImpl.h"
#include "Backend/ModelTreeRepository.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cbasicroot.h"

class TestSystemBackend : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ModelTreeRepository> m_repo;
    std::unique_ptr<SystemBackendImpl> m_backend;
    QString m_dbPath;

private slots:
    void init()
    {
        m_dbPath = QDir::tempPath() + "/test_backend_" +
                   QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        m_repo = std::make_unique<ModelTreeRepository>(m_dbPath,
                   "backend_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        QVERIFY(m_repo->initialize());
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());
    }

    void cleanup()
    {
        m_backend.reset();
        m_repo.reset();
        QFile::remove(m_dbPath);
    }

    void testCreateAccountPersists()
    {
        QString uuid = m_backend->createAccount("TestAccount");
        QVERIFY(!uuid.isEmpty());

        auto rec = m_repo->fetchNode(uuid);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->name, "TestAccount");
        QCOMPARE(rec->modelType, static_cast<int>(ModelType::ACCOUNT));
        QVERIFY(rec->parentUuid.isEmpty());

        QVERIFY(m_backend->dataRoot() != nullptr);
        QCOMPARE(m_backend->dataRoot()->getModels().size(), 1);
    }

    void testCreatePortfolioUnderAccount()
    {
        QString accUuid = m_backend->createAccount("Acct");
        QVERIFY(!accUuid.isEmpty());

        QString portUuid = m_backend->createPortfolio(accUuid, "Port");
        QVERIFY(!portUuid.isEmpty());

        auto rec = m_repo->fetchNode(portUuid);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->parentUuid, accUuid);
        QCOMPARE(rec->modelType, static_cast<int>(ModelType::PORTFOLIO));
    }

    void testCreateStrategyWithPipeline()
    {
        QString accUuid = m_backend->createAccount("Acct");
        QString portUuid = m_backend->createPortfolio(accUuid, "Port");
        QString stratUuid = m_backend->createStrategy(portUuid, ModelType::STRATEGY_PIPELINE);
        QVERIFY(!stratUuid.isEmpty());

        auto rec = m_repo->fetchNode(stratUuid);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->parentUuid, portUuid);
        QCOMPARE(rec->modelType, static_cast<int>(ModelType::STRATEGY_PIPELINE));
    }

    void testRemoveNodeCascades()
    {
        QString accUuid = m_backend->createAccount("Acct");
        QString portUuid = m_backend->createPortfolio(accUuid, "Port");
        QString stratUuid = m_backend->createStrategy(portUuid, ModelType::STRATEGY_PIPELINE);

        QVERIFY(m_backend->removeNode(accUuid));

        QVERIFY(!m_repo->fetchNode(accUuid).has_value());
        QVERIFY(!m_repo->fetchNode(portUuid).has_value());
        QVERIFY(!m_repo->fetchNode(stratUuid).has_value());

        QCOMPARE(m_backend->dataRoot()->getModels().size(), 0);
    }

    void testRenameNodePersists()
    {
        QString uuid = m_backend->createAccount("OldName");
        QVERIFY(m_backend->renameNode(uuid, "NewName"));

        auto rec = m_repo->fetchNode(uuid);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->name, "NewName");
    }

    void testSetActiveNodePersists()
    {
        QString uuid = m_backend->createAccount("Acct");
        QVERIFY(m_backend->setNodeActive(uuid, false));

        auto rec = m_repo->fetchNode(uuid);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->isActive, false);
    }

    void testMoveNodeValid()
    {
        QString acc1 = m_backend->createAccount("Acc1");
        QString acc2 = m_backend->createAccount("Acc2");
        QString port = m_backend->createPortfolio(acc1, "Port");

        QVERIFY(m_backend->moveNode(port, acc2, 0));

        auto rec = m_repo->fetchNode(port);
        QVERIFY(rec.has_value());
        QCOMPARE(rec->parentUuid, acc2);
    }

    void testMoveNodeInvalidType()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        QVERIFY(!m_backend->moveNode(strat, acc, 0));
    }

    void testMoveNodeCycleRejected()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");

        QVERIFY(!m_backend->moveNode(acc, port, 0));
    }

    void testAddBlockPersists()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        QVERIFY(m_backend->addBlock(strat, "Alpha", "MomentumAlpha"));

        QJsonObject cfg = m_backend->pipelineConfig(strat);
        QCOMPARE(cfg["alphas"].toArray().size(), 1);
        QCOMPARE(cfg["alphas"].toArray()[0].toObject()["blockId"].toString(), "MomentumAlpha");
    }

    void testRemoveBlockPersists()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        m_backend->addBlock(strat, "Alpha", "Block1");
        m_backend->addBlock(strat, "Alpha", "Block2");
        QVERIFY(m_backend->removeBlock(strat, "Alpha", 0));

        QJsonObject cfg = m_backend->pipelineConfig(strat);
        QCOMPARE(cfg["alphas"].toArray().size(), 1);
        QCOMPARE(cfg["alphas"].toArray()[0].toObject()["blockId"].toString(), "Block2");
    }

    void testSignalsEmitted()
    {
        QSignalSpy createdSpy(m_backend.get(), &ISystemBackend::nodeCreated);
        QSignalSpy removedSpy(m_backend.get(), &ISystemBackend::nodeRemoved);

        QString uuid = m_backend->createAccount("Acct");
        QCOMPARE(createdSpy.count(), 1);
        QCOMPARE(createdSpy[0][0].toString(), uuid);

        m_backend->removeNode(uuid);
        QCOMPARE(removedSpy.count(), 1);
        QCOMPARE(removedSpy[0][0].toString(), uuid);
    }

    void testPipelineConfigQuery()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        m_backend->addBlock(strat, "Alpha", "TestAlpha");
        m_backend->addBlock(strat, "Risk", "TestRisk");

        QJsonObject cfg = m_backend->pipelineConfig(strat);
        QCOMPARE(cfg["alphas"].toArray().size(), 1);
        QCOMPARE(cfg["risks"].toArray().size(), 1);
    }

    void testNodeInfoReturnsPersistentOnly()
    {
        QString uuid = m_backend->createAccount("InfoTest");
        QJsonObject info = m_backend->nodeInfo(uuid);

        QCOMPARE(info["name"].toString(), "InfoTest");
        QCOMPARE(info["type"].toInt(), static_cast<int>(ModelType::ACCOUNT));
        QVERIFY(!info.contains("genericInfo"));
    }

    void testRuntimeStateReturnedSeparately()
    {
        QString uuid = m_backend->createAccount("RuntimeTest");
        QJsonObject state = m_backend->runtimeState(uuid);
        QVERIFY(state.contains("genericInfo"));
    }

    void testLoadFromDb()
    {
        m_backend->createAccount("AccLoad");

        auto backend2 = std::make_unique<SystemBackendImpl>(m_repo.get());
        QVERIFY(backend2->loadFromDb());
        QCOMPARE(backend2->dataRoot()->getModels().size(), 1);
    }

    void testListAccounts()
    {
        m_backend->createAccount("A1");
        m_backend->createAccount("A2");

        QJsonArray accts = m_backend->listAccounts();
        QCOMPARE(accts.size(), 2);
    }

    void testFullTreeSnapshot()
    {
        QString acc = m_backend->createAccount("Acc");
        m_backend->createPortfolio(acc, "Port");

        QJsonObject snapshot = m_backend->fullTreeSnapshot();
        QVERIFY(snapshot.contains("children"));
        QCOMPARE(snapshot["children"].toArray().size(), 1);
    }
};

#endif // TST_SYSTEM_BACKEND_H
