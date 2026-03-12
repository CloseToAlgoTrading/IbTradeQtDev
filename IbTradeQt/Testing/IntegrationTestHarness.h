#ifndef TESTING_INTEGRATIONTESTHARNESS_H
#define TESTING_INTEGRATIONTESTHARNESS_H

#include <QObject>
#include <QCoreApplication>
#include "Testing/MockMarketDataRouter.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Pipeline/IAlphaBlock.h"
#include "Pipeline/ISelectionBlock.h"
#include "Pipeline/IRebalanceBlock.h"
#include "Pipeline/IRiskBlock.h"
#include "Pipeline/IExecutionBlock.h"
#include "Pipeline/ISignalMergePolicy.h"
#include "Pipeline/Scope.h"

class IntegrationTestHarness : public QObject {
    Q_OBJECT

public:
    explicit IntegrationTestHarness(QObject* parent = nullptr)
        : QObject(parent)
        , m_mockRouter(new MockMarketDataRouter(this))
        , m_mockExecution(new MockExecutionAdapter())
        , m_mockRepo(new MockPositionRepository())
    {}

    ~IntegrationTestHarness() override {
        delete m_mockExecution;
        delete m_mockRepo;
    }

    void runWithTicks(const QVector<IBComm::MarketTick>& ticks) {
        for (const auto& t : ticks) {
            m_mockRouter->simulateTick(t);
        }
    }

    void connectAlphaToRouter(Pipeline::IAlphaBlock* alpha) {
        connect(m_mockRouter, &MockMarketDataRouter::tick,
                alpha, &Pipeline::IAlphaBlock::onTick,
                Qt::DirectConnection);
    }

    // Accessors
    MockMarketDataRouter* router() { return m_mockRouter; }
    MockExecutionAdapter* execution() { return m_mockExecution; }
    MockPositionRepository* repository() { return m_mockRepo; }

    const QVector<Ports::OrderResult>& placedOrders() const {
        return m_mockExecution->placedOrders();
    }

    void reset() {
        m_mockRouter->reset();
        m_mockExecution->reset();
        m_mockRepo->reset();
    }

private:
    MockMarketDataRouter* m_mockRouter;
    MockExecutionAdapter* m_mockExecution;
    MockPositionRepository* m_mockRepo;
};

#endif // TESTING_INTEGRATIONTESTHARNESS_H
