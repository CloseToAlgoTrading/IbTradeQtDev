#ifndef TST_CLI_PROOF_H
#define TST_CLI_PROOF_H

#include <QObject>
#include <QtTest>
#include <QJsonDocument>
#include <QJsonArray>
#include "Backend/SystemBackendImpl.h"
#include "Backend/ModelTreeRepository.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cbasicroot.h"

class TestCliProof : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ModelTreeRepository> m_repo;
    std::unique_ptr<SystemBackendImpl> m_backend;
    QString m_dbPath;

private slots:
    void init()
    {
        m_dbPath = QDir::tempPath() + "/test_cli_" +
                   QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        m_repo = std::make_unique<ModelTreeRepository>(m_dbPath,
                   "cli_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        m_repo->initialize();
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());
    }

    void cleanup()
    {
        m_backend.reset();
        m_repo.reset();
        QFile::remove(m_dbPath);
    }

    void testCliWorkflowNoGui()
    {
        // This test simulates a full CLI workflow without any GUI dependency.
        // It proves that the backend architecture is truly client-independent.

        // 1. Start fresh - no data
        QVERIFY(m_backend->dataRoot() != nullptr);
        QCOMPARE(m_backend->dataRoot()->getModels().size(), 0);

        // 2. Create tree structure via backend (no GUI, no presenter)
        QString acc1 = m_backend->createAccount("TradingAccount");
        QVERIFY(!acc1.isEmpty());

        QString acc2 = m_backend->createAccount("TestAccount");
        QVERIFY(!acc2.isEmpty());

        QString port1 = m_backend->createPortfolio(acc1, "US Equities");
        QVERIFY(!port1.isEmpty());

        QString port2 = m_backend->createPortfolio(acc1, "Crypto");
        QVERIFY(!port2.isEmpty());

        QString strat1 = m_backend->createStrategy(port1, ModelType::STRATEGY_PIPELINE);
        QVERIFY(!strat1.isEmpty());

        // 3. Add blocks to strategy
        QVERIFY(m_backend->addBlock(strat1, "Alpha", "momentum-alpha"));
        QVERIFY(m_backend->addBlock(strat1, "Risk", "max-position-risk"));

        // 4. Inspect tree (like CLI "tree" command)
        QJsonObject snapshot = m_backend->fullTreeSnapshot();
        QJsonArray children = snapshot["children"].toArray();
        QCOMPARE(children.size(), 2);

        // 5. List accounts (like CLI "list" command)
        QJsonArray accts = m_backend->listAccounts();
        QCOMPARE(accts.size(), 2);

        // 6. Query node info (like CLI "info" command)
        QJsonObject info = m_backend->nodeInfo(acc1);
        QCOMPARE(info["name"].toString(), "TradingAccount");

        // 7. Rename (like CLI "rename" command)
        QVERIFY(m_backend->renameNode(acc1, "Main Trading"));
        QJsonObject renamedInfo = m_backend->nodeInfo(acc1);
        QCOMPARE(renamedInfo["name"].toString(), "Main Trading");

        // 8. Remove node (like CLI "remove" command)
        QVERIFY(m_backend->removeNode(acc2));
        QCOMPARE(m_backend->listAccounts().size(), 1);

        // 9. Export (like CLI "export" command)
        QString exportPath = QDir::tempPath() + "/cli_export_test.json";
        QVERIFY(m_backend->exportToJsonFile(exportPath));
        QFile exportFile(exportPath);
        QVERIFY(exportFile.exists());
        QVERIFY(exportFile.size() > 10);
        QFile::remove(exportPath);

        // 10. Verify persistence across "restart"
        m_backend.reset();
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());
        QVERIFY(m_backend->loadFromDb());
        QCOMPARE(m_backend->listAccounts().size(), 1);
        QJsonObject reloadedInfo = m_backend->nodeInfo(acc1);
        QCOMPARE(reloadedInfo["name"].toString(), "Main Trading");

        // 11. Pipeline config survives restart
        QJsonObject pipelineCfg = m_backend->pipelineConfig(strat1);
        QVERIFY(pipelineCfg.contains("alphas"));
        QCOMPARE(pipelineCfg["alphas"].toArray().size(), 1);
    }

    void testCliConnectDisconnectWithoutGui()
    {
        QVERIFY(!m_backend->isBrokerConnected());
        QVERIFY(m_backend->connectBroker());
        QVERIFY(m_backend->isBrokerConnected());
        QVERIFY(m_backend->disconnectBroker());
        QVERIFY(!m_backend->isBrokerConnected());
    }

    void testCliMoveNode()
    {
        QString acc1 = m_backend->createAccount("Acc1");
        QString acc2 = m_backend->createAccount("Acc2");
        QString port = m_backend->createPortfolio(acc1, "MovablePort");

        QCOMPARE(m_backend->listPortfolios(acc1).size(), 1);
        QCOMPARE(m_backend->listPortfolios(acc2).size(), 0);

        QVERIFY(m_backend->moveNode(port, acc2, 0));

        QCOMPARE(m_backend->listPortfolios(acc1).size(), 0);
        QCOMPARE(m_backend->listPortfolios(acc2).size(), 1);
    }
};

#endif // TST_CLI_PROOF_H
