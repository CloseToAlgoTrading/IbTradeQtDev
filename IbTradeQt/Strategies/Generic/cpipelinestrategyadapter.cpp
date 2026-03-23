#include "cpipelinestrategyadapter.h"

#include <QtGlobal>
#include <algorithm>
#include "Common/Expected.h"
#include "GlobalDef.h"
#include "Pipeline/PipelineFactory.h"
#include "Pipeline/PipelineRuntimeContext.h"
#include "Pipeline/PipelineConstants.h"
#include "Pipeline/StrategyPipelineRunner.h"
#include "Pipeline/UniverseResolver.h"
#include "Pipeline/IDataSubscriptionPort.h"
#include "Ports/IPositionRepositoryPort.h"
#include "Supervision/Supervisor.h"
#include "Supervision/StrategyRuntime.h"
#include "IBComm/MarketDataRouter.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMetaObject>
#include <QSet>
#include <QUuid>
#include <QDebug>

IBComm::MarketDataRouter* CPipelineStrategyAdapter::s_globalRouter = nullptr;
Supervision::Supervisor* CPipelineStrategyAdapter::s_globalSupervisor = nullptr;
Ports::IOrderExecutionPort* CPipelineStrategyAdapter::s_globalExecutionPort = nullptr;
Ports::IPositionRepositoryPort* CPipelineStrategyAdapter::s_globalPositionRepo = nullptr;
Ports::IPositionRepositoryPort* CPipelineStrategyAdapter::s_globalPersistentPositionRepo = nullptr;

CPipelineStrategyAdapter::CPipelineStrategyAdapter(QObject* parent)
    : CBaseModel(parent)
{
    MandatoryFieldRegistration::registerStrategyFields(*this);
    m_ParametersMap[MandatoryParams::Name] = QStringLiteral("Pipeline Strategy");
}

CPipelineStrategyAdapter::~CPipelineStrategyAdapter()
{
    stopPipeline();
}

ModelType CPipelineStrategyAdapter::modelType() const { return ModelType::STRATEGY_PIPELINE; }

void CPipelineStrategyAdapter::setGlobalRouter(IBComm::MarketDataRouter* router)
{
    s_globalRouter = router;
}

void CPipelineStrategyAdapter::setGlobalSupervisor(Supervision::Supervisor* supervisor)
{
    s_globalSupervisor = supervisor;
}

void CPipelineStrategyAdapter::setGlobalExecutionPort(Ports::IOrderExecutionPort* port)
{
    s_globalExecutionPort = port;
}

void CPipelineStrategyAdapter::setGlobalPositionRepo(Ports::IPositionRepositoryPort* repo)
{
    s_globalPositionRepo = repo;
}

void CPipelineStrategyAdapter::setGlobalPersistentPositionRepo(Ports::IPositionRepositoryPort* repo)
{
    s_globalPersistentPositionRepo = repo;
}

IBComm::MarketDataRouter* CPipelineStrategyAdapter::globalRouter() { return s_globalRouter; }
Supervision::Supervisor* CPipelineStrategyAdapter::globalSupervisor() { return s_globalSupervisor; }

void CPipelineStrategyAdapter::injectBacktestContext(const BacktestContext& ctx)
{
    m_injectedContext    = ctx;
    m_useInjectedContext = true;
    m_execMode           = ExecutionMode::Backtest;
}

QString CPipelineStrategyAdapter::strategyDefinitionId() const { return m_strategyDefinitionId; }
void CPipelineStrategyAdapter::setStrategyDefinitionId(const QString& defId) { m_strategyDefinitionId = defId; }

CPipelineStrategyAdapter::ExecutionMode CPipelineStrategyAdapter::executionMode() const { return m_execMode; }
void CPipelineStrategyAdapter::setExecutionMode(ExecutionMode mode) { m_execMode = mode; }

void CPipelineStrategyAdapter::setPipelineConfig(const QJsonObject& config)
{
    m_pipelineConfig = config;
    updateParametersFromConfig();
}

const QJsonObject& CPipelineStrategyAdapter::pipelineConfig() const { return m_pipelineConfig; }

Backtest::BacktestProfile CPipelineStrategyAdapter::backtestProfile() const
{
    return Backtest::BacktestProfile::fromJson(
        m_pipelineConfig.value(QStringLiteral("backtestProfile")).toObject());
}

void CPipelineStrategyAdapter::setBacktestProfile(const Backtest::BacktestProfile& profile)
{
    m_pipelineConfig[QStringLiteral("backtestProfile")] = profile.toJson();
    updateParametersFromConfig();
}

void CPipelineStrategyAdapter::loadDefaultConfig(const QString& configPath)
{
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        setPipelineConfig(doc.object());
        if (m_pipelineConfig.contains(QStringLiteral("name"))) {
            setName(m_pipelineConfig[QStringLiteral("name")].toString());
        }
    }
}

void CPipelineStrategyAdapter::refreshMarketUniverseAndSubscriptions()
{
    if (!m_pipelineRunning)
        return;
    const QVector<QString> symbols = computeTradeableSymbolSet();
    Supervision::Supervisor* sup =
        m_useInjectedContext ? m_injectedContext.supervisor : s_globalSupervisor;
    applyUniverseSymbolsToRuntime(sup, symbols);
    if (m_backtestRunner) {
        QStringList qsl;
        for (const QString& s : symbols)
            qsl << s;
        QMetaObject::invokeMethod(m_backtestRunner.get(), "setUniverseFromList", Qt::QueuedConnection,
                                  Q_ARG(QStringList, qsl));
    }
    syncLiveMarketDataSubscriptionsTo();
}

Pipeline::StrategyPipelineRunner* CPipelineStrategyAdapter::backtestPipelineRunner() const
{
    return m_backtestRunner.get();
}

bool CPipelineStrategyAdapter::start()
{
    if (m_pipelineRunning) return true;
    if (m_pipelineConfig.isEmpty()) return false;

    if (m_useInjectedContext) {
        if (m_injectedContext.clock != nullptr) {
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

            {
                Pipeline::PipelineRuntimeContext rtx;
                rtx.subscription = this;
                m_backtestRunner->setRuntimeContext(rtx);
            }

            {
                auto resolved = Pipeline::UniverseResolver::resolve(m_pipelineConfig);
                if (resolved.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols)
                    m_backtestRunner->setUniverse(resolved.symbols);
            }

            m_runtimeName     = getName() + QStringLiteral("_bt_")
                              + m_uuid.toString(QUuid::WithoutBraces).left(8);
            m_pipelineRunning = true;
            return true;
        }

        Q_ASSERT_X(m_injectedContext.router     != nullptr &&
                   m_injectedContext.supervisor  != nullptr &&
                   m_injectedContext.execPort    != nullptr &&
                   m_injectedContext.posRepo     != nullptr,
                   "CPipelineStrategyAdapter::start",
                   "Supervised backtest mode requires fully injected context");

        QString runtimeName = getName() + QStringLiteral("_bt_") + m_uuid.toString(QUuid::WithoutBraces).left(8);
        auto* execPort = m_injectedContext.execPort;
        auto* posRepo  = m_injectedContext.posRepo;
        auto* router   = m_injectedContext.router;

        m_injectedContext.supervisor->addStrategy(
            runtimeName,
            [this, runtimeName, execPort, posRepo, router]() {
                return Pipeline::PipelineFactory::createRuntime(
                    runtimeName, m_pipelineConfig, execPort, posRepo, router, this);
            },
            Supervision::RestartPolicy::Never);

        m_runtimeName     = runtimeName;
        m_pipelineRunning = true;
        updateInfoFromRuntime();
        {
            const QVector<QString> symbols = computeTradeableSymbolSet();
            applyUniverseSymbolsToRuntime(m_injectedContext.supervisor, symbols);
        }
        return true;
    }

    if (s_globalSupervisor && s_globalRouter) {
        QString runtimeName = getName() + QStringLiteral("_") + m_uuid.toString(QUuid::WithoutBraces).left(8);

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
                s_globalRouter,
                this);
            return runtime;
        }, Supervision::RestartPolicy::OnFailure);

        m_runtimeName = runtimeName;
        m_pipelineRunning = true;
        updateInfoFromRuntime();
        {
            const QVector<QString> symbols = computeTradeableSymbolSet();
            applyUniverseSymbolsToRuntime(s_globalSupervisor, symbols);
            warnIfLiveMissingUniverse(symbols);
            syncLiveMarketDataSubscriptionsTo();
        }
    }

    return m_pipelineRunning;
}

bool CPipelineStrategyAdapter::stop()
{
    stopPipeline();
    return true;
}

const QVariantMap& CPipelineStrategyAdapter::getParameters()
{
    updateParametersFromConfig();
    return m_ParametersMap;
}

void CPipelineStrategyAdapter::setParameters(const QVariantMap& parametersMap)
{
    CBaseModel::setParameters(parametersMap);
    updateConfigFromParameters();
}

QVariantMap CPipelineStrategyAdapter::genericInfo() const
{
    QVariantMap info = m_genericInfo;
    info[MandatoryInfo::Strategy::Status] = m_pipelineRunning ? QStringLiteral("Running") : QStringLiteral("Stopped");

    Supervision::Supervisor* activeSupervisor =
        m_useInjectedContext ? m_injectedContext.supervisor : s_globalSupervisor;

    if (activeSupervisor && !m_runtimeName.isEmpty()) {
        auto* rt = activeSupervisor->runtime(m_runtimeName);
        if (rt) {
            info[QStringLiteral("pipeline_runs")] = rt->pipelineRunCount();
            info[QStringLiteral("uptime_ms")] = rt->uptimeMs();
            info[QStringLiteral("healthy")] = rt->isHealthy() ? QStringLiteral("Yes") : QStringLiteral("No");
        }
    }

    int alphaCount = m_pipelineConfig.value(Pipeline::Key::Alphas).toArray().size();
    int riskCount  = m_pipelineConfig.value(Pipeline::Key::Risks).toArray().size();
    info[QStringLiteral("alpha_blocks")] = alphaCount;
    info[QStringLiteral("risk_blocks")] = riskCount;
    info[QStringLiteral("merge_policy")] = m_pipelineConfig.value(QStringLiteral("mergePolicy")).toString(QStringLiteral("none"));
    info[QStringLiteral("execution_mode")] = (m_execMode == ExecutionMode::Live) ? QStringLiteral("live") : QStringLiteral("dry_run");

    return info;
}

QJsonObject CPipelineStrategyAdapter::toJson() const
{
    QJsonObject json = CBaseModel::toJson();
    json[Pipeline::Key::PipelineConfig] = m_pipelineConfig;
    return json;
}

void CPipelineStrategyAdapter::fromJson(const QJsonObject& json)
{
    CBaseModel::fromJson(json);
    if (json.contains(Pipeline::Key::PipelineConfig)) {
        m_pipelineConfig = json[Pipeline::Key::PipelineConfig].toObject();
        updateParametersFromConfig();
    }
    if (json.contains(QStringLiteral("strategyDefinitionId")))
        m_strategyDefinitionId = json[QStringLiteral("strategyDefinitionId")].toString();
}

Contract CPipelineStrategyAdapter::makeUsStockContract(const QString& symbol)
{
    Contract c;
    c.symbol = symbol.toStdString();
    c.secType = "STK";
    c.currency = "USD";
    c.exchange = "SMART";
    return c;
}

QVector<QString> CPipelineStrategyAdapter::computeBaseSymbolList() const
{
    QVector<QString> base;
    QSet<QString> seen;
    auto append = [&](const QString& s) {
        const QString u = s.trimmed().toUpper();
        if (u.isEmpty() || seen.contains(u))
            return;
        seen.insert(u);
        base.append(u);
    };

    const auto resolved = Pipeline::UniverseResolver::resolve(m_pipelineConfig);
    if (resolved.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols
        && !resolved.symbols.isEmpty()) {
        for (const QString& s : resolved.symbols)
            append(s);
    } else {
        const QVariantMap assets = assetList();
        for (auto it = assets.constBegin(); it != assets.constEnd(); ++it) {
            const QString k = it.key();
            if (!k.isEmpty())
                append(k);
        }
    }

    Ports::IPositionRepositoryPort* posRepo = nullptr;
    if (m_useInjectedContext && m_injectedContext.posRepo)
        posRepo = m_injectedContext.posRepo;
    else if (s_globalPositionRepo)
        posRepo = s_globalPositionRepo;
    else if (s_globalPersistentPositionRepo)
        posRepo = s_globalPersistentPositionRepo;

    if (posRepo) {
        const int sid = m_pipelineConfig.value(QStringLiteral("strategyId")).toInt(0);
        const auto positions = posRepo->getAllPositions(sid);
        if (positions.has_value()) {
            for (const auto& row : *positions)
                append(row.symbol);
        }
    }

    const QString bench = backtestProfile().defaultBenchmark.trimmed().toUpper();
    if (!bench.isEmpty())
        append(bench);

    for (const auto& v : m_pipelineConfig.value(QStringLiteral("subscriptionOverlaySymbols")).toArray()) {
        const QString s = v.toString().trimmed().toUpper();
        if (!s.isEmpty())
            append(s);
    }

    return base;
}

QVector<QString> CPipelineStrategyAdapter::computeTradeableSymbolSet() const
{
    return m_subscriptionStore.mergeUnion(computeBaseSymbolList());
}

void CPipelineStrategyAdapter::warnIfLiveMissingUniverse(const QVector<QString>& symbols) const
{
    if (m_execMode != ExecutionMode::Live || !symbols.isEmpty())
        return;
    const auto r = Pipeline::UniverseResolver::resolve(m_pipelineConfig);
    if (r.mode == Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols)
        return;
    qWarning() << "CPipelineStrategyAdapter: Live mode, dynamic selection — no symbols yet "
                  "(fill assetList or call refreshMarketUniverseAndSubscriptions()).";
}

void CPipelineStrategyAdapter::applyUniverseSymbolsToRuntime(Supervision::Supervisor* supervisor,
                                       const QVector<QString>& symbols)
{
    if (!supervisor || m_runtimeName.isEmpty())
        return;
    Supervision::StrategyRuntime* rt = supervisor->runtime(m_runtimeName);
    if (!rt || !rt->runner())
        return;
    QStringList qsl;
    for (const QString& s : symbols)
        qsl << s;
    QMetaObject::invokeMethod(rt->runner(), "setUniverseFromList", Qt::QueuedConnection,
                              Q_ARG(QStringList, qsl));
}

void CPipelineStrategyAdapter::syncLiveMarketDataSubscriptionsTo()
{
    if (m_execMode != ExecutionMode::Live)
        return;
    if (!getIBrokerDataProvider()) {
        qWarning() << "CPipelineStrategyAdapter: Live mode but no broker — skipping market data subscription";
        return;
    }

    const QVector<QString> base = computeBaseSymbolList();
    const QHash<QString, quint32> kinds = m_subscriptionStore.mergeSymbolKindMasks(base);

    QVector<QString> wantTop;
    QVector<QString> wantBars;
    QVector<QString> wantTbt;
    wantTop.reserve(kinds.size());
    for (auto it = kinds.constBegin(); it != kinds.constEnd(); ++it) {
        const QString& sym = it.key();
        const quint32 k = it.value();
        if (k & Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::TopOfBook))
            wantTop.append(sym);
        if (k & Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::RealtimeBars))
            wantBars.append(sym);
        if (k & Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::TickByTick))
            wantTbt.append(sym);
    }
    std::sort(wantTop.begin(), wantTop.end());
    std::sort(wantBars.begin(), wantBars.end());
    std::sort(wantTbt.begin(), wantTbt.end());

    m_coordTop.setDesiredSymbols(wantTop);
    m_coordBars.setDesiredSymbols(wantBars);
    m_coordTbt.setDesiredSymbols(wantTbt);

    QStringList toSub;
    QStringList toCancel;
    m_coordTop.diffAgainstCurrent(m_liveSubscribedTop, &toSub, &toCancel);
    for (const QString& s : toCancel)
        cancelRealTimeData(s);
    for (const QString& s : toSub) {
        reqReadlTimeDataConfigData_t cfg{0, makeUsStockContract(s), QStringLiteral(""), false, false};
        if (!reqestRealTimeData(cfg))
            qWarning() << "CPipelineStrategyAdapter: reqestRealTimeData failed for" << s;
    }
    m_liveSubscribedTop = wantTop;

    m_coordBars.diffAgainstCurrent(m_liveSubscribedRtBars, &toSub, &toCancel);
    for (const QString& s : toCancel)
        cancelRealTimeBars(s);
    for (const QString& s : toSub) {
        if (!requestRealTimeBars(s))
            qWarning() << "CPipelineStrategyAdapter: requestRealTimeBars failed for" << s;
    }
    m_liveSubscribedRtBars = wantBars;

    m_coordTbt.diffAgainstCurrent(m_liveSubscribedTickByTick, &toSub, &toCancel);
    for (const QString& s : toCancel)
        cancelTickByTickData(s);
    for (const QString& s : toSub) {
        reqTickByTickDataConfigData_t cfg{};
        cfg.contract = makeUsStockContract(s);
        cfg.tickType = QStringLiteral("AllLast");
        cfg.numberOfTicks = 0;
        cfg.ignoreSize = false;
        if (!requestTickByTickData(cfg))
            qWarning() << "CPipelineStrategyAdapter: requestTickByTickData failed for" << s;
    }
    m_liveSubscribedTickByTick = wantTbt;
}

void CPipelineStrategyAdapter::unsubscribeLiveMarketData()
{
    for (const QString& sym : m_liveSubscribedTop)
        cancelRealTimeData(sym);
    for (const QString& sym : m_liveSubscribedRtBars)
        cancelRealTimeBars(sym);
    for (const QString& sym : m_liveSubscribedTickByTick)
        cancelTickByTickData(sym);
    m_liveSubscribedTop.clear();
    m_liveSubscribedRtBars.clear();
    m_liveSubscribedTickByTick.clear();
}

void CPipelineStrategyAdapter::stopPipeline()
{
    unsubscribeLiveMarketData();
    if (m_pipelineRunning && !m_runtimeName.isEmpty()) {
        if (m_backtestRunner) {
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
    m_subscriptionStore.clearAll();
}

void CPipelineStrategyAdapter::setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols)
{
    m_subscriptionStore.setDesiredSymbols(ownerId, symbols);
    if (!m_deferSubscriptionRefresh)
        QMetaObject::invokeMethod(this, "onSubscriptionRequestsChanged", Qt::QueuedConnection);
}

void CPipelineStrategyAdapter::setDesiredSymbolsWithKinds(const QString& ownerId,
                                                          const QVector<QString>& symbols,
                                                          quint32 kindMask)
{
    m_subscriptionStore.setDesiredSymbolsWithKinds(ownerId, symbols, kindMask);
    if (!m_deferSubscriptionRefresh)
        QMetaObject::invokeMethod(this, "onSubscriptionRequestsChanged", Qt::QueuedConnection);
}

void CPipelineStrategyAdapter::clearOwner(const QString& ownerId)
{
    m_subscriptionStore.clearOwner(ownerId);
    if (!m_deferSubscriptionRefresh)
        QMetaObject::invokeMethod(this, "onSubscriptionRequestsChanged", Qt::QueuedConnection);
}

void CPipelineStrategyAdapter::clearAll()
{
    m_subscriptionStore.clearAll();
    if (!m_deferSubscriptionRefresh)
        QMetaObject::invokeMethod(this, "onSubscriptionRequestsChanged", Qt::QueuedConnection);
}

void CPipelineStrategyAdapter::beginPipelineEvaluation()
{
    m_subscriptionStore.clearAll();
    m_deferSubscriptionRefresh = true;
}

void CPipelineStrategyAdapter::endPipelineEvaluation()
{
    m_deferSubscriptionRefresh = false;
    refreshMarketUniverseAndSubscriptions();
}

void CPipelineStrategyAdapter::onSubscriptionRequestsChanged()
{
    refreshMarketUniverseAndSubscriptions();
}

void CPipelineStrategyAdapter::updateParametersFromConfig()
{
    QVariantMap preserved;
    for (const auto& key : m_mandatoryParamKeys) {
        if (m_ParametersMap.contains(key))
            preserved[key] = m_ParametersMap[key];
    }
    m_ParametersMap.clear();
    for (auto it = preserved.cbegin(); it != preserved.cend(); ++it)
        m_ParametersMap[it.key()] = it.value();

    if (m_pipelineConfig.contains(QStringLiteral("name")))
        m_ParametersMap[MandatoryParams::Name] = m_pipelineConfig[QStringLiteral("name")].toString();
    if (m_pipelineConfig.contains(QStringLiteral("description")))
        m_ParametersMap[MandatoryParams::Description] = m_pipelineConfig[QStringLiteral("description")].toString();

    if (m_pipelineConfig.contains(QStringLiteral("mergePolicy"))) {
        m_ParametersMap[QStringLiteral("mergePolicy")] = m_pipelineConfig[QStringLiteral("mergePolicy")].toString();
    }

    {
        QJsonObject profileObj = m_pipelineConfig.value(QStringLiteral("backtestProfile")).toObject();
        m_ParametersMap[QStringLiteral("bt_defaultBenchmark")]   = profileObj.value(QStringLiteral("defaultBenchmark")).toString(QStringLiteral("SPY"));
        m_ParametersMap[QStringLiteral("bt_defaultResolution")]  = profileObj.value(QStringLiteral("defaultResolution")).toString(QStringLiteral("Day1"));
        m_ParametersMap[QStringLiteral("bt_defaultDataSource")]  = profileObj.value(QStringLiteral("defaultDataSource")).toString(QStringLiteral("yahoo"));
    }

    QString modeStr;
    switch (m_execMode) {
    case ExecutionMode::Live:     modeStr = QStringLiteral("live");     break;
    case ExecutionMode::Backtest: modeStr = QStringLiteral("backtest"); break;
    default:                      modeStr = QStringLiteral("dry_run");  break;
    }
    m_ParametersMap[QStringLiteral("execution_mode")] = modeStr;
}

void CPipelineStrategyAdapter::updateConfigFromParameters()
{
    if (m_ParametersMap.contains(MandatoryParams::Name)) {
        m_pipelineConfig[QStringLiteral("name")] = m_ParametersMap[MandatoryParams::Name].toString();
    }

    if (m_ParametersMap.contains(QStringLiteral("execution_mode"))) {
        const QString modeStr = m_ParametersMap[QStringLiteral("execution_mode")].toString();
        if (modeStr == QStringLiteral("live"))         m_execMode = ExecutionMode::Live;
        else if (modeStr == QStringLiteral("backtest")) m_execMode = ExecutionMode::Backtest;
        else                            m_execMode = ExecutionMode::DryRun;
    }

    if (m_ParametersMap.contains(QStringLiteral("mergePolicy"))) {
        m_pipelineConfig[QStringLiteral("mergePolicy")] = m_ParametersMap[QStringLiteral("mergePolicy")].toString();
    }

    {
        QJsonObject profileObj = m_pipelineConfig.value(QStringLiteral("backtestProfile")).toObject();
        if (m_ParametersMap.contains(QStringLiteral("bt_defaultBenchmark")))
            profileObj[QStringLiteral("defaultBenchmark")]  = m_ParametersMap[QStringLiteral("bt_defaultBenchmark")].toString();
        if (m_ParametersMap.contains(QStringLiteral("bt_defaultResolution")))
            profileObj[QStringLiteral("defaultResolution")] = m_ParametersMap[QStringLiteral("bt_defaultResolution")].toString();
        if (m_ParametersMap.contains(QStringLiteral("bt_defaultDataSource")))
            profileObj[QStringLiteral("defaultDataSource")] = m_ParametersMap[QStringLiteral("bt_defaultDataSource")].toString();
        m_pipelineConfig[QStringLiteral("backtestProfile")] = profileObj;
    }

    QJsonArray alphas = m_pipelineConfig.value(Pipeline::Key::Alphas).toArray();
    for (int i = 0; i < alphas.size(); ++i) {
        QJsonObject alpha = alphas[i].toObject();
        QJsonObject cfg = alpha.value(Pipeline::Key::Config).toObject();
        QString prefix = QStringLiteral("alpha_%1_").arg(i);

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
        QString prefix = QStringLiteral("risk_%1_").arg(i);

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

void CPipelineStrategyAdapter::updateInfoFromRuntime()
{
    m_genericInfo = genericInfo();
}
