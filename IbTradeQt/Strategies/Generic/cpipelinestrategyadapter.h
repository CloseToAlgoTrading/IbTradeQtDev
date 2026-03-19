#ifndef CPIPELINESTRATEGYADAPTER_H
#define CPIPELINESTRATEGYADAPTER_H

#include "cbasemodel.h"
#include "mandatoryFieldRegistration.h"
#include "mandatoryFieldKeys.h"
#include "Pipeline/PipelineFactory.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/UniverseResolver.h"
#include "Supervision/Supervisor.h"
#include "Supervision/StrategyRuntime.h"
#include "IBComm/MarketDataRouter.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/SimulatedLedger.h"
#include "Common/IClock.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <memory>

class CPipelineStrategyAdapter : public CBaseModel
{
    Q_OBJECT

public:
    enum class ExecutionMode { DryRun, Live, Backtest };

    // Per-instance dependency injection for backtest/test mode.
    //
    // When clock is non-null (pure-backtest path):
    //   start() builds StrategyPipelineRunner directly — no supervisor, no threading.
    //   Required: execPort, clock, ledger.
    //
    // When clock is null and supervisor is non-null (supervised path, legacy tests):
    //   start() delegates to Supervisor::addStrategy() as before.
    //   Required: router, supervisor, execPort, posRepo.
    struct BacktestContext {
        IBComm::MarketDataRouter*        router     = nullptr; // unused in pure-backtest mode
        Supervision::Supervisor*         supervisor = nullptr; // unused in pure-backtest mode
        Ports::IOrderExecutionPort*      execPort   = nullptr; // SimulatedExecutionAdapter*
        Ports::IPositionRepositoryPort*  posRepo    = nullptr; // unused in pure-backtest mode
        IClock*                          clock      = nullptr; // non-null → pure-backtest path
        Backtest::SimulatedLedger*       ledger     = nullptr; // positionRepo for runner
    };

    explicit CPipelineStrategyAdapter(QObject *parent = nullptr)
        : CBaseModel(parent)
    {
        MandatoryFieldRegistration::registerStrategyFields(*this);
        m_ParametersMap[MandatoryParams::Name] = "Pipeline Strategy";
    }

    virtual ~CPipelineStrategyAdapter() override {
        stopPipeline();
    }

    ModelType modelType() const override { return ModelType::STRATEGY_PIPELINE; }

    // --- Static global accessors for app-level singletons ---

    static void setGlobalRouter(IBComm::MarketDataRouter* router) {
        s_globalRouter = router;
    }

    static void setGlobalSupervisor(Supervision::Supervisor* supervisor) {
        s_globalSupervisor = supervisor;
    }

    static void setGlobalExecutionPort(Ports::IOrderExecutionPort* port) {
        s_globalExecutionPort = port;
    }

    static void setGlobalPositionRepo(Ports::IPositionRepositoryPort* repo) {
        s_globalPositionRepo = repo;
    }

    static void setGlobalPersistentPositionRepo(Ports::IPositionRepositoryPort* repo) {
        s_globalPersistentPositionRepo = repo;
    }

    static IBComm::MarketDataRouter* globalRouter() { return s_globalRouter; }
    static Supervision::Supervisor* globalSupervisor() { return s_globalSupervisor; }

    // Inject per-instance context for backtest/test mode.
    // Call this before start(). Automatically sets ExecutionMode::Backtest.
    void injectBacktestContext(const BacktestContext& ctx) {
        m_injectedContext    = ctx;
        m_useInjectedContext = true;
        m_execMode           = ExecutionMode::Backtest;
    }

    // --- Strategy catalog identity (runtime-only, not persisted in config_json) ---
    // Populated by SystemBackendImpl::loadFromDb() from live_strategy_bindings table.
    QString strategyDefinitionId() const { return m_strategyDefinitionId; }
    void setStrategyDefinitionId(const QString& defId) { m_strategyDefinitionId = defId; }

    ExecutionMode executionMode() const { return m_execMode; }
    void setExecutionMode(ExecutionMode mode) { m_execMode = mode; }

    // --- Pipeline config management ---

    void setPipelineConfig(const QJsonObject& config) {
        m_pipelineConfig = config;
        updateParametersFromConfig();
    }

    const QJsonObject& pipelineConfig() const { return m_pipelineConfig; }

    // --- BacktestProfile — lightweight defaults stored per strategy ---
    // Only three fields: defaultBenchmark, defaultResolution, defaultDataSource.
    // All run-specific parameters (dates, capital, slippage) live in the Backtest UI.

    Backtest::BacktestProfile backtestProfile() const {
        return Backtest::BacktestProfile::fromJson(
            m_pipelineConfig.value("backtestProfile").toObject());
    }

    void setBacktestProfile(const Backtest::BacktestProfile& profile) {
        m_pipelineConfig["backtestProfile"] = profile.toJson();
        updateParametersFromConfig();
    }

    void loadDefaultConfig(const QString& configPath) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            setPipelineConfig(doc.object());
            if (m_pipelineConfig.contains("name")) {
                setName(m_pipelineConfig["name"].toString());
            }
        }
    }

    // --- CGenericModelApi overrides ---

    // Non-owning accessor — used by BacktestSession to wire replayer signals after start().
    Pipeline::StrategyPipelineRunner* backtestPipelineRunner() const {
        return m_backtestRunner.get();
    }

    bool start() override {
        if (m_pipelineRunning) return true;
        if (m_pipelineConfig.isEmpty()) return false;

        if (m_useInjectedContext) {
            if (m_injectedContext.clock != nullptr) {
                // ── Pure-backtest mode ──────────────────────────────────────────────
                // Single-threaded, no supervisor. SimulatedClock drives time;
                // SimulatedLedger serves as positionRepo and ledger.
                Q_ASSERT_X(m_injectedContext.execPort != nullptr &&
                           m_injectedContext.ledger   != nullptr,
                           "CPipelineStrategyAdapter::start",
                           "Pure-backtest mode requires execPort and ledger");

                Pipeline::PipelineDefinition def = Pipeline::PipelineFactory::buildDefinition(
                    m_pipelineConfig, m_injectedContext.execPort);

                m_backtestRunner = std::make_unique<Pipeline::StrategyPipelineRunner>(
                    def.graph, def.runtimePolicy,
                    m_injectedContext.execPort, m_injectedContext.ledger,
                    m_injectedContext.clock);
                m_backtestRunner->wireAlphaSignals();

                for (auto* alpha : m_backtestRunner->graph().alphaBlocks)
                    alpha->setClock(m_injectedContext.clock);

                // Seed universe from selection config
                {
                    auto resolved = Pipeline::UniverseResolver::resolve(m_pipelineConfig);
                    if (resolved.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols)
                        m_backtestRunner->setUniverse(resolved.symbols);
                }

                m_runtimeName     = getName() + "_bt_"
                                  + m_uuid.toString(QUuid::WithoutBraces).left(8);
                m_pipelineRunning = true;
                return true;
            }

            // ── Legacy supervised path (used by some existing tests) ────────────
            // Requires router, supervisor, execPort, posRepo all non-null.
            Q_ASSERT_X(m_injectedContext.router     != nullptr &&
                       m_injectedContext.supervisor  != nullptr &&
                       m_injectedContext.execPort    != nullptr &&
                       m_injectedContext.posRepo     != nullptr,
                       "CPipelineStrategyAdapter::start",
                       "Supervised backtest mode requires fully injected context");

            QString runtimeName = getName() + "_bt_" + m_uuid.toString(QUuid::WithoutBraces).left(8);
            auto* execPort = m_injectedContext.execPort;
            auto* posRepo  = m_injectedContext.posRepo;
            auto* router   = m_injectedContext.router;

            m_injectedContext.supervisor->addStrategy(
                runtimeName,
                [this, runtimeName, execPort, posRepo, router]() {
                    return Pipeline::PipelineFactory::createRuntime(
                        runtimeName, m_pipelineConfig, execPort, posRepo, router);
                },
                Supervision::RestartPolicy::Never);

            m_runtimeName     = runtimeName;
            m_pipelineRunning = true;
            updateInfoFromRuntime();
            return true;
        }

        // Live / DryRun path — uses static globals (unchanged behaviour)
        if (s_globalSupervisor && s_globalRouter) {
            QString runtimeName = getName() + "_" + m_uuid.toString(QUuid::WithoutBraces).left(8);

            auto* execPort = (m_execMode == ExecutionMode::Live && s_globalExecutionPort)
                ? s_globalExecutionPort : static_cast<Ports::IOrderExecutionPort*>(&m_mockExecution);
            Ports::IPositionRepositoryPort* posRepo;
            if (m_execMode == ExecutionMode::Live && s_globalPositionRepo)
                posRepo = s_globalPositionRepo;
            else if (s_globalPersistentPositionRepo)
                posRepo = s_globalPersistentPositionRepo;
            else
                posRepo = static_cast<Ports::IPositionRepositoryPort*>(&m_mockPositionRepo);

            s_globalSupervisor->addStrategy(runtimeName, [this, runtimeName, execPort, posRepo]() {
                auto* runtime = Pipeline::PipelineFactory::createRuntime(
                    runtimeName,
                    m_pipelineConfig,
                    execPort,
                    posRepo,
                    s_globalRouter);
                return runtime;
            }, Supervision::RestartPolicy::OnFailure);

            m_runtimeName = runtimeName;
            m_pipelineRunning = true;
            updateInfoFromRuntime();
        }

        return m_pipelineRunning;
    }

    bool stop() override {
        stopPipeline();
        return true;
    }

    const QVariantMap& getParameters() override {
        updateParametersFromConfig();
        return m_ParametersMap;
    }

    void setParameters(const QVariantMap& parametersMap) override {
        CBaseModel::setParameters(parametersMap);
        updateConfigFromParameters();
    }

    QVariantMap genericInfo() const override {
        QVariantMap info = m_genericInfo;
        info[MandatoryInfo::Strategy::Status] = m_pipelineRunning ? "Running" : "Stopped";

        Supervision::Supervisor* activeSupervisor =
            m_useInjectedContext ? m_injectedContext.supervisor : s_globalSupervisor;

        if (activeSupervisor && !m_runtimeName.isEmpty()) {
            auto* rt = activeSupervisor->runtime(m_runtimeName);
            if (rt) {
                info["pipeline_runs"] = rt->pipelineRunCount();
                info["uptime_ms"] = rt->uptimeMs();
                info["healthy"] = rt->isHealthy() ? "Yes" : "No";
            }
        }

        int alphaCount = m_pipelineConfig.value(Pipeline::Key::Alphas).toArray().size();
        int riskCount  = m_pipelineConfig.value(Pipeline::Key::Risks).toArray().size();
        info["alpha_blocks"] = alphaCount;
        info["risk_blocks"] = riskCount;
        info["merge_policy"] = m_pipelineConfig.value("mergePolicy").toString("none");
        info["execution_mode"] = (m_execMode == ExecutionMode::Live) ? "live" : "dry_run";

        return info;
    }

    QJsonObject toJson() const override {
        QJsonObject json = CBaseModel::toJson();
        json[Pipeline::Key::PipelineConfig] = m_pipelineConfig;
        // m_strategyDefinitionId is intentionally NOT written here.
        // The live_strategy_bindings table is the authoritative source.
        return json;
    }

    void fromJson(const QJsonObject& json) override {
        CBaseModel::fromJson(json);
        if (json.contains(Pipeline::Key::PipelineConfig)) {
            m_pipelineConfig = json[Pipeline::Key::PipelineConfig].toObject();
            updateParametersFromConfig();
        }
        // Read as advisory cache only — not authoritative. Authoritative population
        // happens in SystemBackendImpl::loadFromDb() via the bindings table.
        if (json.contains("strategyDefinitionId"))
            m_strategyDefinitionId = json["strategyDefinitionId"].toString();
    }

private:
    void stopPipeline() {
        if (m_pipelineRunning && !m_runtimeName.isEmpty()) {
            if (m_backtestRunner) {
                // Pure-backtest mode: just release the runner
                m_backtestRunner.reset();
            } else {
                Supervision::Supervisor* activeSupervisor =
                    m_useInjectedContext ? m_injectedContext.supervisor : s_globalSupervisor;
                if (activeSupervisor)
                    activeSupervisor->removeStrategy(m_runtimeName);
            }
        }
        m_pipelineRunning = false;
        m_runtimeName.clear();
    }

    // Populate only strategy-level parameters. Block params (alpha_*, risk_*, rebalance_*, execution_*)
    // belong to the respective model containers and are not aggregated here.
    void updateParametersFromConfig() {
        QVariantMap preserved;
        for (const auto& key : m_mandatoryParamKeys) {
            if (m_ParametersMap.contains(key))
                preserved[key] = m_ParametersMap[key];
        }
        m_ParametersMap.clear();
        for (auto it = preserved.cbegin(); it != preserved.cend(); ++it)
            m_ParametersMap[it.key()] = it.value();

        if (m_pipelineConfig.contains("name"))
            m_ParametersMap[MandatoryParams::Name] = m_pipelineConfig["name"].toString();
        if (m_pipelineConfig.contains("description"))
            m_ParametersMap[MandatoryParams::Description] = m_pipelineConfig["description"].toString();

        if (m_pipelineConfig.contains("mergePolicy")) {
            m_ParametersMap["mergePolicy"] = m_pipelineConfig["mergePolicy"].toString();
        }

        // BacktestProfile defaults — exposed in tree so users can set sensible defaults
        // without needing to re-enter them every time they open the Backtest Workspace.
        {
            QJsonObject profileObj = m_pipelineConfig.value("backtestProfile").toObject();
            m_ParametersMap["bt_defaultBenchmark"]   = profileObj.value("defaultBenchmark").toString("SPY");
            m_ParametersMap["bt_defaultResolution"]  = profileObj.value("defaultResolution").toString("Day1");
            m_ParametersMap["bt_defaultDataSource"]  = profileObj.value("defaultDataSource").toString("yahoo");
        }

        QString modeStr;
        switch (m_execMode) {
        case ExecutionMode::Live:     modeStr = "live";     break;
        case ExecutionMode::Backtest: modeStr = "backtest"; break;
        default:                      modeStr = "dry_run";  break;
        }
        m_ParametersMap["execution_mode"] = modeStr;
    }

    void updateConfigFromParameters() {
        if (m_ParametersMap.contains(MandatoryParams::Name)) {
            m_pipelineConfig["name"] = m_ParametersMap[MandatoryParams::Name].toString();
        }

        if (m_ParametersMap.contains("execution_mode")) {
            const QString modeStr = m_ParametersMap["execution_mode"].toString();
            if (modeStr == "live")         m_execMode = ExecutionMode::Live;
            else if (modeStr == "backtest") m_execMode = ExecutionMode::Backtest;
            else                            m_execMode = ExecutionMode::DryRun;
        }

        if (m_ParametersMap.contains("mergePolicy")) {
            m_pipelineConfig["mergePolicy"] = m_ParametersMap["mergePolicy"].toString();
        }

        // Write BacktestProfile defaults back to pipelineConfig JSON
        {
            QJsonObject profileObj = m_pipelineConfig.value("backtestProfile").toObject();
            if (m_ParametersMap.contains("bt_defaultBenchmark"))
                profileObj["defaultBenchmark"]  = m_ParametersMap["bt_defaultBenchmark"].toString();
            if (m_ParametersMap.contains("bt_defaultResolution"))
                profileObj["defaultResolution"] = m_ParametersMap["bt_defaultResolution"].toString();
            if (m_ParametersMap.contains("bt_defaultDataSource"))
                profileObj["defaultDataSource"] = m_ParametersMap["bt_defaultDataSource"].toString();
            m_pipelineConfig["backtestProfile"] = profileObj;
        }

        QJsonArray alphas = m_pipelineConfig.value(Pipeline::Key::Alphas).toArray();
        for (int i = 0; i < alphas.size(); ++i) {
            QJsonObject alpha = alphas[i].toObject();
            QJsonObject cfg = alpha.value(Pipeline::Key::Config).toObject();
            QString prefix = QString("alpha_%1_").arg(i);

            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                QString key = prefix + it.key();
                if (m_ParametersMap.contains(key)) {
                    cfg[it.key()] = QJsonValue::fromVariant(m_ParametersMap[key]);
                }
            }
            alpha[Pipeline::Key::Config] = cfg;
            alphas[i] = alpha;
        }
        m_pipelineConfig[Pipeline::Key::Alphas] = alphas;

        QJsonArray risks = m_pipelineConfig.value(Pipeline::Key::Risks).toArray();
        for (int i = 0; i < risks.size(); ++i) {
            QJsonObject risk = risks[i].toObject();
            QJsonObject cfg = risk.value(Pipeline::Key::Config).toObject();
            QString prefix = QString("risk_%1_").arg(i);

            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                QString key = prefix + it.key();
                if (m_ParametersMap.contains(key)) {
                    cfg[it.key()] = QJsonValue::fromVariant(m_ParametersMap[key]);
                }
            }
            risk[Pipeline::Key::Config] = cfg;
            risks[i] = risk;
        }
        m_pipelineConfig[Pipeline::Key::Risks] = risks;
    }

    void updateInfoFromRuntime() {
        m_genericInfo = genericInfo();
    }

    QJsonObject m_pipelineConfig;
    bool m_pipelineRunning = false;
    QString m_runtimeName;
    ExecutionMode m_execMode = ExecutionMode::DryRun;

    // Per-instance backtest context (set via injectBacktestContext())
    BacktestContext m_injectedContext;
    bool m_useInjectedContext = false;

    // Pure-backtest runner — non-null only after start() in pure-backtest mode.
    // BacktestSession accesses this via backtestPipelineRunner().
    std::unique_ptr<Pipeline::StrategyPipelineRunner> m_backtestRunner;

    // Runtime-only identity field. Not persisted in config_json.
    // Populated by SystemBackendImpl::loadFromDb() from live_strategy_bindings table.
    // live_strategy_bindings is the authoritative source for the node→definition mapping.
    QString m_strategyDefinitionId;

    MockExecutionAdapter m_mockExecution;
    MockPositionRepository m_mockPositionRepo;

    static inline IBComm::MarketDataRouter* s_globalRouter = nullptr;
    static inline Supervision::Supervisor* s_globalSupervisor = nullptr;
    static inline Ports::IOrderExecutionPort* s_globalExecutionPort = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPositionRepo = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPersistentPositionRepo = nullptr;
};

#endif // CPIPELINESTRATEGYADAPTER_H
