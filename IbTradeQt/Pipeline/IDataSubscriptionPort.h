#ifndef PIPELINE_IDATASUBSCRIPTIONPORT_H
#define PIPELINE_IDATASUBSCRIPTIONPORT_H

#include <QString>
#include <QVector>

namespace Pipeline {

/// Bit flags for broker streaming (OR together). TopOfBook = reqMktData path.
enum class SubscriptionKind : quint32 {
    TopOfBook     = 1u,
    RealtimeBars  = 2u,
    TickByTick    = 4u,
};

inline quint32 subscriptionKindMask(SubscriptionKind k)
{
    return static_cast<quint32>(k);
}

/// Blocks request streaming symbols here; strategy adapter merges and applies broker subscriptions.
/// Wire-level subscribe/unsubscribe stays in CPipelineStrategyAdapter (or coordinator).
class IDataSubscriptionPort {
public:
    virtual ~IDataSubscriptionPort() = default;

    /// Default: TopOfBook only (same as legacy single reqMktData per symbol).
    virtual void setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols) = 0;

    /// Same kind mask applied to every symbol (OR of SubscriptionKind bits).
    virtual void setDesiredSymbolsWithKinds(const QString& ownerId, const QVector<QString>& symbols,
                                            quint32 kindMask) = 0;

    virtual void clearOwner(const QString& ownerId) = 0;
    virtual void clearAll() = 0;

    /// One pipeline evaluation: clear owner contributions without broker refresh; blocks refill during the run.
    virtual void beginPipelineEvaluation() = 0;
    /// Single merge + refresh after pipeline step completes.
    virtual void endPipelineEvaluation() = 0;
};

} // namespace Pipeline

#endif
