#pragma once

#include <QSharedPointer>
#include <QString>
#include <QStringList>

class IBrokerAPI;

namespace Brokers {

/// Central factory for broker API implementations. Currently: `ib` (Interactive Brokers), `paper` (stub).
QSharedPointer<IBrokerAPI> createBrokerApi(const QString& backendId);

QStringList supportedBrokerBackends();

} // namespace Brokers
