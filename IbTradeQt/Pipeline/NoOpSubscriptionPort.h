#ifndef PIPELINE_NOOPSUBSCRIPTIONPORT_H
#define PIPELINE_NOOPSUBSCRIPTIONPORT_H

#include "IDataSubscriptionPort.h"

namespace Pipeline {

/// Backtest sessions / tests: IDataSubscriptionPort with no broker side-effects.
class NoOpSubscriptionPort : public IDataSubscriptionPort {
public:
    void setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols) override;
    void setDesiredSymbolsWithKinds(const QString& ownerId, const QVector<QString>& symbols,
                                    quint32 kindMask) override;
    void clearOwner(const QString& ownerId) override;
    void clearAll() override;
    void beginPipelineEvaluation() override;
    void endPipelineEvaluation() override;
};

} // namespace Pipeline

#endif
