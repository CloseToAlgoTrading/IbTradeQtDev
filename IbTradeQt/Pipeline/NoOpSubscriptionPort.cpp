#include "NoOpSubscriptionPort.h"

namespace Pipeline {

void NoOpSubscriptionPort::setDesiredSymbols(const QString&, const QVector<QString>&) {}
void NoOpSubscriptionPort::setDesiredSymbolsWithKinds(const QString&, const QVector<QString>&, quint32) {}
void NoOpSubscriptionPort::clearOwner(const QString&) {}
void NoOpSubscriptionPort::clearAll() {}
void NoOpSubscriptionPort::beginPipelineEvaluation() {}
void NoOpSubscriptionPort::endPipelineEvaluation() {}

} // namespace Pipeline
