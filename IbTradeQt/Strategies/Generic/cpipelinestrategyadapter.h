#ifndef CPIPELINESTRATEGYADAPTER_H
#define CPIPELINESTRATEGYADAPTER_H

#include "cbasemodel.h"
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
    enum class ExecutionMode { DryRun, Live };

    explicit CPipelineStrategyAdapter(QObject *parent = nullptr)
        : CBaseModel(parent)
    {
        m_Name = "Pipeline Strategy";
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
                m_Name = m_pipelineConfig["name"].toString();
            }
        }
    }

    // --- CGenericModelApi overrides ---

    bool start() override {
        if (m_pipelineRunning) return true;
        if (m_pipelineConfig.isEmpty()) return false;

        if (s_globalSupervisor && s_globalRouter) {
            QString runtimeName = m_Name + "_" + m_uuid.toString(QUuid::WithoutBraces).left(8);

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
        m_ParametersMap = parametersMap;
        updateConfigFromParameters();
    }

    QVariantMap genericInfo() const override {
        QVariantMap info;
        info["type"] = "LEGO Pipeline";
        info["status"] = m_pipelineRunning ? "Running" : "Stopped";

        if (s_globalSupervisor && !m_runtimeName.isEmpty()) {
            auto* rt = s_globalSupervisor->runtime(m_runtimeName);
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
        if (m_pipelineRunning && s_globalSupervisor && !m_runtimeName.isEmpty()) {
            s_globalSupervisor->removeStrategy(m_runtimeName);
        }
        m_pipelineRunning = false;
        m_runtimeName.clear();
    }

    void updateParametersFromConfig() {
        m_ParametersMap.clear();

        if (m_pipelineConfig.contains("name")) {
            m_ParametersMap["pipeline_name"] = m_pipelineConfig["name"].toString();
        }
        if (m_pipelineConfig.contains("description")) {
            m_ParametersMap["pipeline_description"] = m_pipelineConfig["description"].toString();
        }

        QJsonArray alphas = m_pipelineConfig.value("alphas").toArray();
        for (int i = 0; i < alphas.size(); ++i) {
            QJsonObject alpha = alphas[i].toObject();
            QString prefix = QString("alpha_%1_").arg(i);
            m_ParametersMap[prefix + "blockId"] = alpha.value("blockId").toString();

            QJsonObject cfg = alpha.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                m_ParametersMap[prefix + it.key()] = it.value().toVariant();
            }
        }

        QJsonObject rebalance = m_pipelineConfig.value("rebalance").toObject();
        if (!rebalance.isEmpty()) {
            m_ParametersMap["rebalance_blockId"] = rebalance.value("blockId").toString();
            QJsonObject cfg = rebalance.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                m_ParametersMap["rebalance_" + it.key()] = it.value().toVariant();
            }
        }

        QJsonArray risks = m_pipelineConfig.value("risks").toArray();
        for (int i = 0; i < risks.size(); ++i) {
            QJsonObject risk = risks[i].toObject();
            QString prefix = QString("risk_%1_").arg(i);
            m_ParametersMap[prefix + "blockId"] = risk.value("blockId").toString();

            QJsonObject cfg = risk.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                m_ParametersMap[prefix + it.key()] = it.value().toVariant();
            }
        }

        QJsonObject exec = m_pipelineConfig.value("execution").toObject();
        if (!exec.isEmpty()) {
            m_ParametersMap["execution_blockId"] = exec.value("blockId").toString();
            QJsonObject cfg = exec.value("config").toObject();
            for (auto it = cfg.begin(); it != cfg.end(); ++it) {
                m_ParametersMap["execution_" + it.key()] = it.value().toVariant();
            }
        }

        if (m_pipelineConfig.contains("mergePolicy")) {
            m_ParametersMap["mergePolicy"] = m_pipelineConfig["mergePolicy"].toString();
        }

        m_ParametersMap["execution_mode"] = (m_execMode == ExecutionMode::Live) ? "live" : "dry_run";
    }

    void updateConfigFromParameters() {
        if (m_ParametersMap.contains("pipeline_name")) {
            m_pipelineConfig["name"] = m_ParametersMap["pipeline_name"].toString();
            m_Name = m_ParametersMap["pipeline_name"].toString();
        }

        if (m_ParametersMap.contains("execution_mode")) {
            m_execMode = (m_ParametersMap["execution_mode"].toString() == "live")
                ? ExecutionMode::Live : ExecutionMode::DryRun;
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

    MockExecutionAdapter m_mockExecution;
    MockPositionRepository m_mockPositionRepo;

    static inline IBComm::MarketDataRouter* s_globalRouter = nullptr;
    static inline Supervision::Supervisor* s_globalSupervisor = nullptr;
    static inline Ports::IOrderExecutionPort* s_globalExecutionPort = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPositionRepo = nullptr;
    static inline Ports::IPositionRepositoryPort* s_globalPersistentPositionRepo = nullptr;
};

#endif // CPIPELINESTRATEGYADAPTER_H
