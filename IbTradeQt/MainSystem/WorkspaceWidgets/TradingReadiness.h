#ifndef TRADINGREADINESS_H
#define TRADINGREADINESS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace TradingUX {

struct ReadinessState {
    bool brokerConnected = false;
    bool brokerConnecting = false;
    bool liveOrders = false;
    bool strategyRunning = false;
    bool requiredPortsAvailable = true;
    bool dataSourceSelected = true;
    bool cacheCoverageKnown = false;
    bool cacheCoverageMissing = false;
    int resolvedSymbolCount = 0;
    QString dataSourceId;
    QStringList blockingMessages;
    QStringList warningMessages;

    bool hasBlocks() const { return !blockingMessages.isEmpty(); }
    QString summaryText() const;
};

ReadinessState computeLiveReadiness(const QJsonObject& pipelineConfig,
                                    bool liveOrders,
                                    bool strategyRunning,
                                    bool brokerConnected,
                                    bool brokerConnecting,
                                    bool requiredPortsAvailable);

ReadinessState computeBacktestReadiness(const QStringList& symbols,
                                        const QString& dataSourceId,
                                        bool brokerConnected,
                                        bool coverageKnown,
                                        bool coverageMissing);

QString brokerText(const ReadinessState& state);
QString executionText(const ReadinessState& state);
QString dataText(const ReadinessState& state);
QString universeText(const ReadinessState& state);
QString strategyText(const ReadinessState& state);

} // namespace TradingUX

#endif // TRADINGREADINESS_H
