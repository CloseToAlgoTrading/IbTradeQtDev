#ifndef ADAPTERS_IBHISTORICALDATAFETCHER_H
#define ADAPTERS_IBHISTORICALDATAFETCHER_H

#include "IBComm/HistoricalDataRouter.h"

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <functional>

class CBrokerDataProvider;

namespace Adapters {

struct IBHistoricalFetchRequest {
    QStringList symbols;
    QString resolution;
    QDateTime fromUtc;
    QDateTime toUtc;
    int timeoutMs = 60000;
    QHash<QString, QVariantMap> assetBySymbol;
    std::function<bool()> cancelRequested;
};

struct IBHistoricalFetchResult {
    QVector<IBComm::HistoricalBar> bars;
    QStringList failedSymbols;
    QString errorMessage;
};

class IBHistoricalDataFetcher {
public:
    explicit IBHistoricalDataFetcher(CBrokerDataProvider* broker);

    IBHistoricalFetchResult fetch(const IBHistoricalFetchRequest& request) const;

    static QString ibBarSizeFromResolution(const QString& resolution);
    static QString ibDurationFromRange(const QDateTime& fromUtc, const QDateTime& toUtc);

private:
    CBrokerDataProvider* m_broker = nullptr;
};

} // namespace Adapters

#endif // ADAPTERS_IBHISTORICALDATAFETCHER_H
