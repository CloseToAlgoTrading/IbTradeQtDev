#ifndef CPIPELINESTRATEGYADAPTER_H
#define CPIPELINESTRATEGYADAPTER_H

#include "cbasemodel.h"
#include "mandatoryFieldRegistration.h"
#include "mandatoryFieldKeys.h"
#include "Pipeline/PipelineFactory.h"
#include "Supervision/Supervisor.h"
#include "Supervision/StrategyRuntime.h"
#include "IBComm/MarketDataRouter.h"
#include "Adapters/MockExecutionAdapter.h"
#include "Adapters/MockPositionRepository.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

class CPipelineStrategyAdapter : public CBaseModel
{
    Q_OBJECT

public:
    enum class ExecutionMode { DryRun, Live, Backtest };

    // Per-instance dependency injection for backtest mode.
    // All fields must be non-null when used with ExecutionMode::Backtest.
    struct BacktestContext {
        IBComm::MarketDataRouter*        router     = nullptr;
        Supervision::Supervisor*         supervisor = nullptr;
        Ports::IOrderExecutionPort*      execPort   = nullptr;
        Ports::IPositionRepositoryPort*  posRepo    = nullptr;
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

    // Inject per-instance context for backtest mode.
    // Call this before start(). Automatically sets ExecutionMode::Backtest.
    void injectBacktestContext(const BacktestContext& ctx) {
        m_injectedContext    = ctx;
        m_useInjectedContext = true;
        m_execMode           = ExecutionMode::Backtest;
    }

    ExecutionMode executionMode() const { return m_execMode; }
    void setExecutionMode(ExecutionMode mode) { m_execMode = mode; }

    // --- Pipeline config management ---

    void setPipelineConfig(const QJsonObject& config) {
        m_pipelineConfig = config;
        updateParametersFromConfig();
    }

    const QJsonObject& pipelineConfig() const { return m_pipelineConfig; }

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

    bool start() override {
        if (m_pipelineRunning) return true;
        if (m_pipelineConfig.isEmpty()) return false;

        if (m_useInjectedContext) {
            // Backtest mode — must use injected context; static globals are forbidden
            Q_ASSERT_X(m_injectedContext.router     != nullptr &&
                       m_injectedContext.supervisor  != nullptr &&
                       m_injectedContext.execPort    != nullptr &&
                       m_injectedContext.posRepo     != nullptr,
                       "CPipelineStrategyAdapter::start",
                       "Backtest mode requires fully injected context — "
                       "all BacktestContext fields must be non-null");

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

        int alphaCount = m_pipelineConfig.value("alphas").toArray().size();
        int riskCount = m_pipelineConfig.value("risks").toArray().size();
        info["alpha_blocks"] = alphaCount;
        info["risk_blocks"] = riskCount;
        info["merge_policy"] = m_pipelineConfig.value("mergePolicy").toString("none");
        info["execution_mode"] = (m_execMode == ExecutionMode::Live) ? "live" : "dry_run";

        return info;
    }

    QJsonObject toJson() const override {
        QJsonObject json = CBaseModel::toJson();
        json["pipelineConfig"] = m_pipelineConfig;
        return json;
    }

    void fromJson(const QJsonObject& json) override {
        CBaseModel::fromJson(json);
        if (json.contains("pipelineConfig")) {
            m_pipelineConfig = json["pipelineConfig"].toObject();
            updateParametersFromConfig();
        }
    }

private:
    void stopPipeline() {
        if (m_pipelineRunning && !m_runtimeName.isEmpty()) {
            Supervision::Supervisor* activeSupervisor =
                m_useInjectedContext ? m_injectedContext.supervisor : s_globalSupervisor;
            if (activeSupervisor) {
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

        QJsonArray alphas = m_pipelineConfig.value("alphas").toArray();
        for (int i = 0; i < alphas.size(); ++i) {
            QJsonObject alpha = alphas[i].toObject();
            QJsonObject cfg = alpha.value("config").toObject();
            QString prefix = QString("alpha_%1_").arg(i);

            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                QString key = prefix + it.key();
                if (m_ParametersMap.contains(key)) {
                    cfg[it.key()] = QJsonValue::fromVariant(m_ParametersMap[key]);
                }
            }
            alpha["config"] = cfg;
            alphas[i] = alpha;
        }
        m_pipelineConfig["alphas"] = alphas;

        QJsonArray risks = m_pipelineConfig.value("risks").toArray();
        for (int i = 0; i < risks.size(); ++i) {
            QJsonObject risk = risks[i].toObject();
            QJsonObject cfg = risk.value("config").toObject();
            QString prefix = QString("risk_%1_").arg(i);

            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                QString key = prefix + it.key();
                if (m_ParametersMap.contains(key)) {
                    cfg[it.key()] = QJsonValue::fromVariant(m_ParametersMap[key]);
                }
            }
            risk["config"] = cfg;
            risks[i] = risk;
        }
        m_pipelineConfig["risks"] = risks;
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

    MockExecutionAdapter m_mockExecution;
    MockPositionRepository m_mockPositionRepo;

    static inline IBComm::MarketDataRouter* s_globalRouter = nullptr;
    static inline Supervision::Supervisor* s_globalSupervisor = nullptr;
    static inline Ports::IOrderExecutionPort* s_globalExecutionPort = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPositionRepo = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPersistentPositionRepo = nullptr;
};

#endif // CPIPELINESTRATEGYADAPTER_H
