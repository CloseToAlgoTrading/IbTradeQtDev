#ifndef TST_RUNTIME_H
#define TST_RUNTIME_H

#include <QObject>
#include <QtTest>
#include <QSignalSpy>
#include "Backend/SystemBackendImpl.h"
#include "Backend/ModelTreeRepository.h"
#include "Backend/ModelTreeMapper.h"
#include "Strategies/Generic/ModelType.h"
#include "Strategies/Generic/cbasicroot.h"
#include "Strategies/Generic/cpipelinestrategyadapter.h"

class TestRuntime : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<ModelTreeRepository> m_repo;
    std::unique_ptr<SystemBackendImpl> m_backend;
    QString m_dbPath;

private slots:
    void init()
    {
        m_dbPath = QDir::tempPath() + "/test_runtime_" +
                   QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
        m_repo = std::make_unique<ModelTreeRepository>(m_dbPath,
                   "runtime_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        m_repo->initialize();
        m_backend = std::make_unique<SystemBackendImpl>(m_repo.get());
    }

    void cleanup()
    {
        m_backend.reset();
        m_repo.reset();
        QFile::remove(m_dbPath);
    }

    void testBrokerConnectionState()
    {
        QVERIFY(!m_backend->isBrokerConnected());
        QVERIFY(m_backend->connectBroker());
        QVERIFY(m_backend->isBrokerConnected());
        QVERIFY(m_backend->disconnectBroker());
        QVERIFY(!m_backend->isBrokerConnected());
    }

    void testBrokerConnectionSignals()
    {
        QSignalSpy spy(m_backend.get(), &ISystemBackend::brokerConnectionChanged);
        m_backend->connectBroker();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy[0][0].toBool(), true);

        m_backend->disconnectBroker();
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy[1][0].toBool(), false);
    }

    void testRuntimeStateQuerySeparateFromNodeInfo()
    {
        QString uuid = m_backend->createAccount("RuntimeAcc");

        QJsonObject info = m_backend->nodeInfo(uuid);
        QVERIFY(!info.contains("genericInfo"));

        QJsonObject state = m_backend->runtimeState(uuid);
        QVERIFY(state.contains("genericInfo"));
    }

    void testRuntimeStateDoesNotPersist()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        auto rec = m_repo->fetchNode(strat);
        QVERIFY(rec.has_value());

        QVERIFY(!rec->config.contains("genericInfo"));

        QString configStr = QJsonDocument(rec->config).toJson();
        QVERIFY(!configStr.contains("displayState"));
        QVERIFY(!configStr.contains("heartbeat"));
    }

    void testNodeConfigJsonStripsRuntimeData()
    {
        QString acc = m_backend->createAccount("StrippedAcc");

        auto rec = m_repo->fetchNode(acc);
        QVERIFY(rec.has_value());

        QVERIFY(!rec->config.contains("models"));
        QVERIFY(!rec->config.contains("genericInfo"));
    }

    void testBacktesterGetsPipelineConfigFromBackend()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        m_backend->addBlock(strat, "Alpha", "momentum-alpha");
        m_backend->addBlock(strat, "Risk", "max-position-risk");

        QJsonObject config = m_backend->pipelineConfig(strat);

        QVERIFY(config.contains("alphas"));
        QVERIFY(config.contains("risks"));
        QCOMPARE(config["alphas"].toArray().size(), 1);
        QCOMPARE(config["risks"].toArray().size(), 1);
    }

    void testPipelineConfigPersistedInDb()
    {
        QString acc = m_backend->createAccount("Acc");
        QString port = m_backend->createPortfolio(acc, "Port");
        QString strat = m_backend->createStrategy(port, ModelType::STRATEGY_PIPELINE);

        m_backend->addBlock(strat, "Alpha", "momentum-alpha");

        auto rec = m_repo->fetchNode(strat);
        QVERIFY(rec.has_value());

        QString cfgStr = QJsonDocument(rec->config).toJson();
        QVERIFY(cfgStr.contains("momentum-alpha"));
    }

    void testPipelineConfigSurvivesRestart()
    {
        QString stratId;
        {
            auto repo2 = std::make_unique<ModelTreeRepository>(m_dbPath,
                "restart_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
            repo2->initialize();
            auto backend2 = std::make_unique<SystemBackendImpl>(repo2.get());
            QString acc = backend2->createAccount("Acc");
            QString port = backend2->createPortfolio(acc, "Port");
            stratId = backend2->createStrategy(port, ModelType::STRATEGY_PIPELINE);
            backend2->addBlock(stratId, "Alpha", "momentum-alpha");
        }

        // New session
        auto repo3 = std::make_unique<ModelTreeRepository>(m_dbPath,
            "restart2_conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        repo3->initialize();
        auto backend3 = std::make_unique<SystemBackendImpl>(repo3.get());
        QVERIFY(backend3->loadFromDb());

        QJsonObject config = backend3->pipelineConfig(stratId);
        QVERIFY(config.contains("alphas"));
        QCOMPARE(config["alphas"].toArray().size(), 1);
    }
};

#endif // TST_RUNTIME_H
