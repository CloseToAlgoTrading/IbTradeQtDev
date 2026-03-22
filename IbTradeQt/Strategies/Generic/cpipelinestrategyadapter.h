#ifndef CPIPELINESTRATEGYADAPTER_H
#define CPIPELINESTRATEGYADAPTER_H

#include "cbasemodel.h"
#include "mandatoryFieldRegistration.h"
#include "mandatoryFieldKeys.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/SimulatedLedger.h"
#include "Common/IClock.h"
#include "GlobalDef.h"
#include <QJsonObject>
#include <QMetaObject>
#include <QSet>
#include <memory>

namespace IBComm {
class MarketDataRouter;
}

namespace Pipeline {
class StrategyPipelineRunner;
}

namespace Ports {
class IOrderExecutionPort;
class IPositionRepositoryPort;
}

namespace Supervision {
class Supervisor;
class StrategyRuntime;
}

class CPipelineStrategyAdapter : public CBaseModel
{
    Q_OBJECT

public:
    enum class ExecutionMode { DryRun, Live, Backtest };

    struct BacktestContext {
        IBComm::MarketDataRouter*        router     = nullptr;
        Supervision::Supervisor*         supervisor = nullptr;
        Ports::IOrderExecutionPort*      execPort   = nullptr;
        Ports::IPositionRepositoryPort*  posRepo    = nullptr;
        IClock*                          clock      = nullptr;
        Backtest::SimulatedLedger*       ledger     = nullptr;
    };

    explicit CPipelineStrategyAdapter(QObject* parent = nullptr);
    ~CPipelineStrategyAdapter() override;

    ModelType modelType() const override;

    static void setGlobalRouter(IBComm::MarketDataRouter* router);
    static void setGlobalSupervisor(Supervision::Supervisor* supervisor);
    static void setGlobalExecutionPort(Ports::IOrderExecutionPort* port);
    static void setGlobalPositionRepo(Ports::IPositionRepositoryPort* repo);
    static void setGlobalPersistentPositionRepo(Ports::IPositionRepositoryPort* repo);

    static IBComm::MarketDataRouter* globalRouter();
    static Supervision::Supervisor* globalSupervisor();

    void injectBacktestContext(const BacktestContext& ctx);

    QString strategyDefinitionId() const;
    void setStrategyDefinitionId(const QString& defId);

    ExecutionMode executionMode() const;
    void setExecutionMode(ExecutionMode mode);

    void setPipelineConfig(const QJsonObject& config);
    const QJsonObject& pipelineConfig() const;

    Backtest::BacktestProfile backtestProfile() const;
    void setBacktestProfile(const Backtest::BacktestProfile& profile);

    void loadDefaultConfig(const QString& configPath);

    void refreshMarketUniverseAndSubscriptions();

    Pipeline::StrategyPipelineRunner* backtestPipelineRunner() const;

    bool start() override;
    bool stop() override;

    const QVariantMap& getParameters() override;
    void setParameters(const QVariantMap& parametersMap) override;

    QVariantMap genericInfo() const override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

private:
    static Contract makeUsStockContract(const QString& symbol);

    QVector<QString> computeTradeableSymbolSet() const;
    void warnIfLiveMissingUniverse(const QVector<QString>& symbols) const;
    void applyUniverseSymbolsToRuntime(Supervision::Supervisor* supervisor,
                                       const QVector<QString>& symbols);
    void syncLiveMarketDataSubscriptionsTo(const QVector<QString>& desired);
    void unsubscribeLiveMarketData();
    void stopPipeline();
    void updateParametersFromConfig();
    void updateConfigFromParameters();
    void updateInfoFromRuntime();

    QJsonObject m_pipelineConfig;
    bool m_pipelineRunning = false;
    QString m_runtimeName;
    ExecutionMode m_execMode = ExecutionMode::DryRun;

    BacktestContext m_injectedContext;
    bool m_useInjectedContext = false;

    std::unique_ptr<Pipeline::StrategyPipelineRunner> m_backtestRunner;

    QString m_strategyDefinitionId;

    MockExecutionAdapter m_mockExecution;
    MockPositionRepository m_mockPositionRepo;

    QVector<QString> m_liveSubscribedSymbols;

    static IBComm::MarketDataRouter* s_globalRouter;
    static Supervision::Supervisor* s_globalSupervisor;
    static Ports::IOrderExecutionPort* s_globalExecutionPort;
    static Ports::IPositionRepositoryPort* s_globalPositionRepo;
    static Ports::IPositionRepositoryPort* s_globalPersistentPositionRepo;
};

#endif // CPIPELINESTRATEGYADAPTER_H
