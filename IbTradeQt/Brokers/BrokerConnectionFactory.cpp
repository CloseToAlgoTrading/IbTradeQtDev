#include "BrokerConnectionFactory.h"
#include "PaperBrokerStub.h"
#include "IBComClientImpl.h"

namespace Brokers {

QSharedPointer<IBrokerAPI> createBrokerApi(const QString& backendId)
{
    const QString id = backendId.trimmed().toLower();
    if (id.isEmpty() || id == QStringLiteral("ib"))
        return QSharedPointer<IBrokerAPI>(new IBComClientImpl);
    if (id == QStringLiteral("paper"))
        return QSharedPointer<IBrokerAPI>(new PaperBrokerStub);
    return {};
}

QStringList supportedBrokerBackends()
{
    return {QStringLiteral("ib"), QStringLiteral("paper")};
}

} // namespace Brokers
