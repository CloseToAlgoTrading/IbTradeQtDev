#include "IBHistoricalDataFetcher.h"

#include "IBComm/cbrokerdataprovider.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QSet>
#include <QTimer>

Q_LOGGING_CATEGORY(lcIBHistoricalFetcher, "adapters.ibHistoricalFetcher")

namespace Adapters {

namespace {

struct IBContractSpec {
    QString secType = QStringLiteral("STK");
    QString currency = QStringLiteral("USD");
    QString exchange = QStringLiteral("SMART");
    QString primaryExchange;
    QString whatToShow = QStringLiteral("TRADES");
    int useRth = 1;
};

QString firstNonEmpty(const QVariantMap& values, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QString value = values.value(QString::fromUtf8(key)).toString().trimmed();
        if (!value.isEmpty())
            return value;
    }
    return {};
}

IBContractSpec contractSpecForSymbol(const QString& symbol,
                                     const QHash<QString, QVariantMap>& assetBySymbol)
{
    IBContractSpec spec;
    const QVariantMap asset = assetBySymbol.value(symbol);

    const QString secType = firstNonEmpty(asset, {"secType", "SecurityType", "securityType"});
    const QString currency = firstNonEmpty(asset, {"currency", "Currency"});
    const QString exchange = firstNonEmpty(asset, {"exchange", "Exchange", "market", "Market"});
    const QString primary = firstNonEmpty(asset, {"primaryExchange", "PrimaryExchange", "listingExchange"});
    const QString whatToShow = firstNonEmpty(asset, {"whatToShow", "WhatToShow"});

    if (!secType.isEmpty())
        spec.secType = secType.toUpper();
    if (!currency.isEmpty())
        spec.currency = currency.toUpper();
    if (!exchange.isEmpty())
        spec.exchange = exchange.toUpper();
    if (!primary.isEmpty())
        spec.primaryExchange = primary.toUpper();
    if (!whatToShow.isEmpty())
        spec.whatToShow = whatToShow.toUpper();
    if (asset.contains(QStringLiteral("useRth")))
        spec.useRth = asset.value(QStringLiteral("useRth")).toInt();

    // Accept lightweight symbol hints without requiring a broker selector UI yet:
    // "NASDAQ:MSFT" or "MSFT@NASDAQ" becomes primaryExchange=NASDAQ, symbol=MSFT.
    Q_UNUSED(symbol)
    return spec;
}

QString normalizedSymbol(QString rawSymbol, IBContractSpec* spec)
{
    QString symbol = rawSymbol.trimmed().toUpper();
    const int colon = symbol.indexOf(QLatin1Char(':'));
    if (colon > 0 && colon + 1 < symbol.size()) {
        if (spec && spec->primaryExchange.isEmpty())
            spec->primaryExchange = symbol.left(colon);
        symbol = symbol.mid(colon + 1);
    }
    const int at = symbol.indexOf(QLatin1Char('@'));
    if (at > 0 && at + 1 < symbol.size()) {
        if (spec && spec->primaryExchange.isEmpty())
            spec->primaryExchange = symbol.mid(at + 1);
        symbol = symbol.left(at);
    }
    return symbol;
}

qint64 chunkSecondsForResolution(const QString& resolution)
{
    const QString r = resolution.trimmed().toUpper();
    if (r == QStringLiteral("DAY1") || r == QStringLiteral("1D") || r.contains(QStringLiteral("DAY")))
        return 365LL * 24LL * 60LL * 60LL;
    if (r.contains(QStringLiteral("HOUR")) || r == QStringLiteral("1H"))
        return 30LL * 24LL * 60LL * 60LL;
    if (r == QStringLiteral("MIN30") || r == QStringLiteral("30MIN")
        || r == QStringLiteral("MIN15") || r == QStringLiteral("15MIN"))
        return 14LL * 24LL * 60LL * 60LL;
    if (r == QStringLiteral("MIN5") || r == QStringLiteral("5MIN"))
        return 7LL * 24LL * 60LL * 60LL;
    if (r.contains(QStringLiteral("MIN")) || r == QStringLiteral("1M"))
        return 24LL * 60LL * 60LL;
    if (r == QStringLiteral("SEC5") || r == QStringLiteral("5SEC"))
        return 2LL * 60LL * 60LL;
    return 365LL * 24LL * 60LL * 60LL;
}

QString ibEndDateTime(const QDateTime& utc)
{
    return utc.toUTC().toString(QStringLiteral("yyyyMMdd-HH:mm:ss"));
}

} // namespace

IBHistoricalDataFetcher::IBHistoricalDataFetcher(CBrokerDataProvider* broker)
    : m_broker(broker)
{}

QString IBHistoricalDataFetcher::ibBarSizeFromResolution(const QString& resolution)
{
    const QString r = resolution.trimmed().toUpper();
    if (r == QStringLiteral("DAY1") || r == QStringLiteral("1D") || r.contains(QStringLiteral("DAY")))
        return QStringLiteral("1 day");
    if (r.contains(QStringLiteral("HOUR")) || r == QStringLiteral("1H"))
        return QStringLiteral("1 hour");
    if (r == QStringLiteral("MIN30") || r == QStringLiteral("30MIN"))
        return QStringLiteral("30 mins");
    if (r == QStringLiteral("MIN15") || r == QStringLiteral("15MIN"))
        return QStringLiteral("15 mins");
    if (r == QStringLiteral("MIN5") || r == QStringLiteral("5MIN"))
        return QStringLiteral("5 mins");
    if (r.contains(QStringLiteral("MIN")) || r == QStringLiteral("1M"))
        return QStringLiteral("1 min");
    if (r == QStringLiteral("SEC5") || r == QStringLiteral("5SEC"))
        return QStringLiteral("5 secs");
    return QStringLiteral("1 day");
}

QString IBHistoricalDataFetcher::ibDurationFromRange(const QDateTime& fromUtc, const QDateTime& toUtc)
{
    if (!fromUtc.isValid() || !toUtc.isValid())
        return QStringLiteral("30 D");
    const qint64 secs = qMax<qint64>(1, fromUtc.secsTo(toUtc));
    if (secs < 86400)
        return QString::number(secs) + QStringLiteral(" S");
    const int days = qMax(1, qMin(365 * 5, static_cast<int>((secs + 86399) / 86400)));
    return QString::number(days) + QStringLiteral(" D");
}

IBHistoricalFetchResult IBHistoricalDataFetcher::fetch(const IBHistoricalFetchRequest& request) const
{
    IBHistoricalFetchResult result;
    if (!m_broker || !m_broker->getClien() || !m_broker->historicalDataRouter()) {
        result.errorMessage = QStringLiteral("IB broker or historical router is not available.");
        return result;
    }
    if (!m_broker->isConnectedToTheServer()) {
        result.errorMessage = QStringLiteral("IB broker is not connected.");
        return result;
    }

    if (request.cancelRequested && request.cancelRequested()) {
        result.errorMessage = QStringLiteral("IB historical request cancelled.");
        return result;
    }

    const QString barSize = ibBarSizeFromResolution(request.resolution);
    IBComm::HistoricalDataRouter* router = m_broker->historicalDataRouter();
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    const QDateTime rangeFrom = request.fromUtc.toUTC();
    const QDateTime rangeTo = qMin(request.toUtc.toUTC(), nowUtc);
    if (!rangeFrom.isValid() || !rangeTo.isValid() || rangeFrom >= rangeTo) {
        result.errorMessage = QStringLiteral("IB historical request has an invalid or future-only range.");
        return result;
    }
    const qint64 maxChunkSecs = chunkSecondsForResolution(request.resolution);

    for (const QString& rawSymbol : request.symbols) {
        IBContractSpec spec = contractSpecForSymbol(rawSymbol.trimmed().toUpper(), request.assetBySymbol);
        const QString symbol = normalizedSymbol(rawSymbol, &spec);
        if (symbol.isEmpty())
            continue;

        QVector<IBComm::HistoricalBar> symbolBars;
        QDateTime chunkFrom = rangeFrom;
        bool symbolFailed = false;
        bool cancelled = false;

        while (chunkFrom < rangeTo) {
            if (request.cancelRequested && request.cancelRequested()) {
                cancelled = true;
                break;
            }

            const QDateTime chunkTo = qMin(chunkFrom.addSecs(maxChunkSecs), rangeTo);
            reqHistConfigData_t cfg(0, barSize, ibDurationFromRange(chunkFrom, chunkTo), symbol);
            cfg.endDateTimeUtc = ibEndDateTime(chunkTo);
            cfg.secType = spec.secType;
            cfg.currency = spec.currency;
            cfg.exchange = spec.exchange;
            cfg.primaryExchange = spec.primaryExchange;
            cfg.whatToShow = spec.whatToShow;
            cfg.useRth = spec.useRth;

            QVector<IBComm::HistoricalBar> chunkBars;
            QEventLoop loop;
            int reqId = -1;
            bool timedOut = false;

            QMetaObject::Connection conn = QObject::connect(
                router,
                &IBComm::HistoricalDataRouter::barsReceived,
                &loop,
                [&](int id, const QString&, const QVector<IBComm::HistoricalBar>& bars) {
                    if (id == reqId) {
                        chunkBars = bars;
                        loop.quit();
                    }
                },
                Qt::QueuedConnection);

            if (!m_broker->reqestHistoricalData(cfg)) {
                QObject::disconnect(conn);
                symbolFailed = true;
                break;
            }
            reqId = cfg.id;

            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            timeoutTimer.setInterval(qMax(1000, request.timeoutMs));
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
                timedOut = true;
                loop.quit();
            });

            QTimer cancelTimer;
            cancelTimer.setInterval(250);
            QObject::connect(&cancelTimer, &QTimer::timeout, &loop, [&]() {
                if (request.cancelRequested && request.cancelRequested()) {
                    cancelled = true;
                    loop.quit();
                }
            });

            timeoutTimer.start();
            cancelTimer.start();
            loop.exec();

            QObject::disconnect(conn);
            if ((timedOut || cancelled) && reqId > 0)
                m_broker->cancelHistoricalData(reqId);

            if (cancelled)
                break;
            if (timedOut) {
                qCWarning(lcIBHistoricalFetcher) << "IB historical request timed out for" << symbol
                                                 << cfg.endDateTimeUtc << cfg.duration << cfg.barSize;
                symbolFailed = true;
                break;
            }

            for (IBComm::HistoricalBar bar : chunkBars) {
                if (bar.symbol.isEmpty())
                    bar.symbol = symbol;
                const QDateTime ts = bar.timestamp.toUTC();
                if (ts >= rangeFrom && ts <= rangeTo)
                    symbolBars.append(bar);
            }
            chunkFrom = chunkTo.addSecs(1);
        }

        if (cancelled) {
            result.errorMessage = QStringLiteral("IB historical request cancelled.");
            return result;
        }

        if (symbolBars.isEmpty() || symbolFailed) {
            qCWarning(lcIBHistoricalFetcher) << "IB historical request returned no bars for" << symbol;
            result.failedSymbols.append(symbol);
            continue;
        }

        QMap<QDateTime, IBComm::HistoricalBar> deduped;
        for (const auto& bar : symbolBars)
            deduped[bar.timestamp.toUTC()] = bar;
        for (auto it = deduped.begin(); it != deduped.end(); ++it) {
            IBComm::HistoricalBar bar = it.value();
            bar.timestamp = bar.timestamp.toUTC();
            result.bars.append(bar);
        }
    }

    if (result.bars.isEmpty() && result.errorMessage.isEmpty() && !request.symbols.isEmpty())
        result.errorMessage = QStringLiteral("IB historical request returned no bars.");

    return result;
}

} // namespace Adapters
