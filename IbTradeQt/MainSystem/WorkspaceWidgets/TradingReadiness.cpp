#include "TradingReadiness.h"

#include "Pipeline/UniverseResolver.h"

namespace TradingUX {

namespace {

QStringList resolvedSymbolsFromPipeline(const QJsonObject& pipelineConfig)
{
    QStringList symbols;
    const auto resolved = Pipeline::UniverseResolver::resolve(pipelineConfig);
    if (resolved.mode != Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols)
        return symbols;
    for (const QString& sym : resolved.symbols) {
        const QString u = sym.trimmed().toUpper();
        if (!u.isEmpty() && !symbols.contains(u))
            symbols.append(u);
    }
    return symbols;
}

} // namespace

QString ReadinessState::summaryText() const
{
    QStringList lines;
    if (!blockingMessages.isEmpty())
        lines << QStringLiteral("Blocked: %1").arg(blockingMessages.join(QStringLiteral("; ")));
    if (!warningMessages.isEmpty())
        lines << QStringLiteral("Check: %1").arg(warningMessages.join(QStringLiteral("; ")));
    if (lines.isEmpty())
        lines << QStringLiteral("Ready");
    return lines.join(QLatin1Char('\n'));
}

ReadinessState computeLiveReadiness(const QJsonObject& pipelineConfig,
                                    bool liveOrders,
                                    bool strategyRunning,
                                    bool brokerConnected,
                                    bool brokerConnecting,
                                    bool requiredPortsAvailable)
{
    ReadinessState s;
    s.brokerConnected = brokerConnected;
    s.brokerConnecting = brokerConnecting;
    s.liveOrders = liveOrders;
    s.strategyRunning = strategyRunning;
    s.requiredPortsAvailable = requiredPortsAvailable;
    s.dataSourceId = QStringLiteral("ib");
    s.resolvedSymbolCount = resolvedSymbolsFromPipeline(pipelineConfig).size();

    if (liveOrders && !brokerConnected)
        s.blockingMessages << QStringLiteral("Connect IB/TWS before live orders");
    if (liveOrders && !requiredPortsAvailable)
        s.blockingMessages << QStringLiteral("Live order and position ports are unavailable");
    if (liveOrders && s.resolvedSymbolCount <= 0)
        s.blockingMessages << QStringLiteral("No tradeable symbols resolved");
    if (!liveOrders)
        s.warningMessages << QStringLiteral("Dry run: orders are simulated");
    else if (brokerConnected)
        s.warningMessages << QStringLiteral("Live: orders will be sent to IB/TWS");
    return s;
}

ReadinessState computeBacktestReadiness(const QStringList& symbols,
                                        const QString& dataSourceId,
                                        bool brokerConnected,
                                        bool coverageKnown,
                                        bool coverageMissing)
{
    ReadinessState s;
    s.brokerConnected = brokerConnected;
    s.dataSourceId = dataSourceId.trimmed().toLower();
    s.dataSourceSelected = !s.dataSourceId.isEmpty();
    s.cacheCoverageKnown = coverageKnown;
    s.cacheCoverageMissing = coverageMissing;
    s.resolvedSymbolCount = symbols.size();

    if (!s.dataSourceSelected)
        s.blockingMessages << QStringLiteral("Choose a data source");
    if (s.resolvedSymbolCount <= 0)
        s.blockingMessages << QStringLiteral("No symbols resolved");
    if (s.dataSourceId == QLatin1String("ib") && !brokerConnected) {
        if (coverageKnown && !coverageMissing)
            s.warningMessages << QStringLiteral("IB/TWS disconnected; cached data can still run");
        else
            s.warningMessages << QStringLiteral("Requires active IB/TWS connection for missing data");
    }
    if (coverageKnown) {
        s.warningMessages << (coverageMissing
            ? QStringLiteral("Coverage has gaps")
            : QStringLiteral("Coverage checked"));
    } else {
        s.warningMessages << QStringLiteral("Coverage not checked");
    }
    return s;
}

QString brokerText(const ReadinessState& state)
{
    if (state.brokerConnecting)
        return QStringLiteral("Broker: connecting to IB/TWS");
    if (state.brokerConnected)
        return QStringLiteral("Broker: IB connected");
    return QStringLiteral("Broker: disconnected");
}

QString executionText(const ReadinessState& state)
{
    return state.liveOrders
        ? QStringLiteral("Execution: live orders")
        : QStringLiteral("Execution: dry run");
}

QString dataText(const ReadinessState& state)
{
    if (state.dataSourceId.isEmpty())
        return QStringLiteral("Data: --");
    if (state.dataSourceId == QLatin1String("ib"))
        return state.brokerConnected ? QStringLiteral("Data: IB/TWS") : QStringLiteral("Data: IB/TWS requires connection");
    return QStringLiteral("Data: %1").arg(state.dataSourceId);
}

QString universeText(const ReadinessState& state)
{
    return QStringLiteral("Universe: %1 symbol(s)").arg(state.resolvedSymbolCount);
}

QString strategyText(const ReadinessState& state)
{
    return state.strategyRunning ? QStringLiteral("Strategy: running") : QStringLiteral("Strategy: stopped");
}

} // namespace TradingUX
