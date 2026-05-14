#include "YahooFinanceDataSource.h"
#include "Backtest/InstrumentClassification.h"
#include "Backtest/InstrumentClassificationMappers.h"
#include "Backtest/InstrumentInference.h"
#include "Backtest/InstrumentMetadataResolver.h"
#include "Backtest/InstrumentNormalization.h"
#include "Backtest/MarketSessionUtils.h"
#include "DB/dbdatatypes.h"
#include "DB/dbquery.h"
#include <QDateTime>
#include <QJsonDocument> // meta JSON for InstrumentMetadata.rawJson
#include <QSqlError>
#include <QSqlQuery>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QTimeZone>
#include <QLoggingCategory>
#include <QDate>
#include <QTime>

Q_LOGGING_CATEGORY(lcYahoo, "backtest.yahoo")

namespace Backtest {

namespace {

void upsertYahooInstrumentMetadata(const QString& dbConn, const QString& symbol, const QJsonObject& r0)
{
    if (dbConn.isEmpty())
        return;

    const QJsonObject meta = r0.value(QStringLiteral("meta")).toObject();
    const QString     it   = meta.value(QStringLiteral("instrumentType")).toString();
    const QString     qt   = meta.value(QStringLiteral("quoteType")).toString();
    AssetKind         ak   = assetKindFromYahooInstrumentType(it, qt);
    if (ak == AssetKind::Unknown)
        ak = inferAssetKindFromSymbolHeuristic(symbol);

    DbInstrumentMetadata row;
    row.providerSymbol = normalizeProviderSymbol(QStringLiteral("yahoo"), symbol);
    row.providerId     = QStringLiteral("yahoo");
    row.assetKind      = assetKindToString(ak);
    row.sourceRawType  = it;
    row.currency       = meta.value(QStringLiteral("currency")).toString();
    row.exchange       = meta.value(QStringLiteral("exchangeName")).toString();
    row.displayName         = meta.value(QStringLiteral("shortName")).toString();
    row.tradingScheduleId   = meta.value(QStringLiteral("exchangeTimezoneName")).toString();
    row.updatedAt      = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QJsonDocument jd(meta);
    QString       rj = QString::fromUtf8(jd.toJson(QJsonDocument::Compact));
    if (rj.size() > 4096)
        rj.resize(4096);
    row.rawJson = rj;

    QSqlQuery q = query_upsertInstrumentMetadata(row, dbConn);
    if (!q.exec())
        qCWarning(lcYahoo) << "YahooFinanceDataSource: InstrumentMetadata upsert failed"
                           << q.lastError().text();
}

// Yahoo chart interval=1d: period1 = inclusive start (UTC midnight of start date),
// period2 = exclusive end (UTC midnight of day after end date).
struct YahooEpochBounds {
    qint64 period1 = 0;
    qint64 period2 = 0;
    bool   valid   = false;
};

YahooEpochBounds computeYahooChartEpochBounds(const QDateTime& from, const QDateTime& to)
{
    YahooEpochBounds b;
    const qint64 nowSec = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    const QDate  todayUtc = QDateTime::currentDateTimeUtc().date();

    if (!from.isValid() && !to.isValid()) {
        b.period1 = 0;
        b.period2 = nowSec;
        b.valid   = b.period2 > b.period1;
        return b;
    }
    if (!from.isValid() && to.isValid()) {
        b.period1 = 0;
        QDate d = to.toUTC().date();
        if (d > todayUtc)
            d = todayUtc;
        b.period2 = QDateTime(d.addDays(1), QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
        b.valid   = b.period2 > b.period1;
        return b;
    }
    if (from.isValid() && !to.isValid()) {
        const QDate d0 = from.toUTC().date();
        b.period1 = QDateTime(d0, QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
        b.period2 = qMax(nowSec, b.period1 + 86400);
        b.valid   = b.period2 > b.period1;
        return b;
    }
    QDate d1 = from.toUTC().date();
    QDate d2 = to.toUTC().date();
    // Yahoo has no daily bars after "today" in UTC; clamp a mis-set future end date.
    if (d2 > todayUtc)
        d2 = todayUtc;
    if (d1 > d2) {
        b.valid = false;
        return b;
    }
    b.period1 = QDateTime(d1, QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
    b.period2 = QDateTime(d2.addDays(1), QTime(0, 0), Qt::UTC).toSecsSinceEpoch();
    b.valid   = b.period2 > b.period1;
    return b;
}

/// Filter range for emitting bars (matches Yahoo daily bar timestamps within calendar days).
void setFilterBoundsFromRequest(QDateTime& outFrom, QDateTime& outTo,
                                const QDateTime& from, const QDateTime& to)
{
    if (!from.isValid() && !to.isValid()) {
        outFrom = from;
        outTo   = to;
        return;
    }
    if (from.isValid() && to.isValid()) {
        const QDate d1 = from.toUTC().date();
        const QDate d2 = to.toUTC().date();
        outFrom = QDateTime(d1, QTime(0, 0), Qt::UTC);
        outTo   = QDateTime(d2, QTime(23, 59, 59), Qt::UTC).addMSecs(999);
        return;
    }
    if (from.isValid() && !to.isValid()) {
        outFrom = QDateTime(from.toUTC().date(), QTime(0, 0), Qt::UTC);
        outTo   = QDateTime();
        return;
    }
    outFrom = QDateTime();
    outTo   = QDateTime(to.toUTC().date(), QTime(23, 59, 59), Qt::UTC).addMSecs(999);
}

} // namespace

// Yahoo Finance v8 chart API — returns JSON with OHLCV arrays.
// Endpoint: https://query1.finance.yahoo.com/v8/finance/chart/{symbol}
// Query params: interval, period1, period2, events=history
static const QString kYahooBaseUrl =
    "https://query1.finance.yahoo.com/v8/finance/chart/%1";


YahooFinanceDataSource::YahooFinanceDataSource(QObject* parent)
    : IHistoricalDataSource(parent)
{}

void YahooFinanceDataSource::setNetworkManager(QNetworkAccessManager* mgr)
{
    if (m_ownNam && m_nam) {
        m_nam->deleteLater();
    }
    m_nam    = mgr;
    m_ownNam = false;
}

void YahooFinanceDataSource::disconnectFinishedHandler()
{
    if (!m_nam)
        return;
    QObject::disconnect(m_nam, &QNetworkAccessManager::finished,
                        this, &YahooFinanceDataSource::onReplyFinished);
}

void YahooFinanceDataSource::setInstrumentMetadataDbConnection(const QString& dbConnectionName)
{
    m_instrumentMetadataDbConnection = dbConnectionName;
}

void YahooFinanceDataSource::requestBars(const QStringList& symbols,
                                         const QDateTime& from,
                                         const QDateTime& to,
                                         BarResolution resolution)
{
    QStringList syms;
    syms.reserve(symbols.size());
    for (const QString& s : symbols) {
        const QString t = s.trimmed();
        if (t.isEmpty()) {
            qCWarning(lcYahoo) << "YahooFinanceDataSource: skipping empty symbol in request";
            continue;
        }
        syms.append(t);
    }
    if (syms.isEmpty()) {
        emit loadFinished();
        return;
    }

    if (!m_nam) {
        m_nam    = new QNetworkAccessManager(this);
        m_ownNam = true;
    }
    // Connect finished signal (idempotent — Qt deduplicates identical connections)
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &YahooFinanceDataSource::onReplyFinished,
            Qt::UniqueConnection);

    m_requestedSymbols = syms;
    setFilterBoundsFromRequest(m_from, m_to, from, to);
    m_pendingCount = 0;
    m_failedSymbols.clear();
    m_lastError.clear();

    // Yahoo Finance free API only supports daily data reliably for multi-year ranges.
    // Intraday intervals are capped to ~60 days by Yahoo; always use daily.
    if (resolution != BarResolution::Day1) {
        qCWarning(lcYahoo) << "YahooFinanceDataSource: resolution"
                   << static_cast<int>(resolution)
                   << "not supported — falling back to Day1 (Yahoo Finance limit)";
    }
    const QString interval = "1d";

    const YahooEpochBounds bounds = computeYahooChartEpochBounds(from, to);
    if (!bounds.valid) {
        qCWarning(lcYahoo) << "YahooFinanceDataSource: Yahoo request skipped: invalid period range"
                           << "period1=" << bounds.period1 << "period2=" << bounds.period2;
        emit loadFinished();
        return;
    }

    const qint64 period1 = bounds.period1;
    const qint64 period2 = bounds.period2;

    for (const QString& symbol : syms) {
        QUrl url(kYahooBaseUrl.arg(symbol));
        QUrlQuery query;
        query.addQueryItem("interval",  interval);
        query.addQueryItem("period1",   QString::number(period1));
        query.addQueryItem("period2",   QString::number(period2));
        query.addQueryItem("events",    "history");
        url.setQuery(query);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 (compatible; IbTradeQt/1.0)");
        req.setAttribute(QNetworkRequest::User, symbol);

        qCDebug(lcYahoo) << "YahooFinanceDataSource: GET" << url.toString();
        m_nam->get(req);
        ++m_pendingCount;
    }
}

void YahooFinanceDataSource::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString symbol = reply->request().attribute(QNetworkRequest::User).toString();
        const QVariant statusVar = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        const QByteArray body      = reply->readAll();
        const QString    bodyPrev  = QString::fromUtf8(body.left(512));

        const QString errorMsg = QStringLiteral("HTTP/transport failure: %1").arg(reply->errorString());
        m_lastError = QStringLiteral("HTTP/transport failure for %1: %2")
                          .arg(symbol, reply->errorString());
        const int httpStatus = statusVar.toInt();
        if (httpStatus == 404) {
            qCWarning(lcYahoo) << "YahooFinanceDataSource: chart HTTP 404 for" << symbol
                               << "— Yahoo has no chart data (wrong ticker, delisted, or no bars in range)."
                               << "qtError=" << static_cast<int>(reply->error())
                               << "url=" << reply->url().toString()
                               << "body=" << bodyPrev;
        } else {
            qCWarning(lcYahoo) << "YahooFinanceDataSource: HTTP/transport failure for" << symbol
                               << "qtError=" << static_cast<int>(reply->error())
                               << "httpStatus=" << statusVar
                               << "url=" << reply->url().toString()
                               << "body=" << bodyPrev;
        }
        
        m_failedSymbols.insert(symbol);
        emit symbolFailed(symbol, errorMsg);
        
        --m_pendingCount;
        checkAllDone();
        return;
    }

    parseChartReply(reply);
    --m_pendingCount;
    checkAllDone();
}

void YahooFinanceDataSource::parseChartReply(QNetworkReply* reply)
{
    const QString symbol = reply->request().attribute(QNetworkRequest::User).toString();
    const QByteArray raw = reply->readAll();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        qCWarning(lcYahoo) << "YahooFinanceDataSource: JSON parse error for" << symbol
                   << parseErr.errorString();
        return;
    }

    // Navigate: chart -> result[0] -> { timestamp[], indicators.quote[0].{open,high,low,close,volume} }
    const QJsonObject root   = doc.object();
    const QJsonObject chart  = root["chart"].toObject();
    const QJsonArray  result = chart["result"].toArray();

    if (result.isEmpty()) {
        const QString errMsg = chart["error"].toObject()["description"].toString("unknown");
        qCWarning(lcYahoo) << "YahooFinanceDataSource: no result for" << symbol << "—" << errMsg;
        return;
    }

    const QJsonObject r0         = result[0].toObject();
    const QJsonArray  timestamps = r0["timestamp"].toArray();
    const QJsonObject indicators = r0["indicators"].toObject();
    const QJsonArray  quoteArr   = indicators["quote"].toArray();

    if (quoteArr.isEmpty() || timestamps.isEmpty()) {
        const QUrl    url = reply->url();
        QUrlQuery     uq(url);
        const qint64  p1  = uq.queryItemValue(QStringLiteral("period1")).toLongLong();
        const qint64  p2  = uq.queryItemValue(QStringLiteral("period2")).toLongLong();

        upsertYahooInstrumentMetadata(m_instrumentMetadataDbConnection, symbol, r0);

        const AssetKind kForSession =
            InstrumentMetadataResolver::resolve(m_instrumentMetadataDbConnection, symbol, QStringLiteral("yahoo"), {})
                .effectiveAssetKind;
        if (appliesYahooUsCashEquitySessionDaily(kForSession) && isWeekendOnlyChartWindowUtcNy(p1, p2)) {
            qCDebug(lcYahoo) << "YahooFinanceDataSource: no daily bars (weekend window, US equity) for"
                             << symbol << "url=" << url.toString();
        } else {
            qCWarning(lcYahoo) << "YahooFinanceDataSource: empty quote data for" << symbol;
        }
        return;
    }

    const QJsonObject quote  = quoteArr[0].toObject();
    const QJsonArray  opens  = quote["open"].toArray();
    const QJsonArray  highs  = quote["high"].toArray();
    const QJsonArray  lows   = quote["low"].toArray();
    const QJsonArray  closes = quote["close"].toArray();
    const QJsonArray  vols   = quote["volume"].toArray();

    int count = timestamps.size();
    int emitted = 0;

    for (int i = 0; i < count; ++i) {
        // Yahoo timestamps are Unix seconds (UTC)
        const qint64 ts = static_cast<qint64>(timestamps[i].toDouble());
        QDateTime dt = QDateTime::fromSecsSinceEpoch(ts, QTimeZone::utc());

        // Skip rows with null values (Yahoo sometimes returns null for holidays)
        if (closes[i].isNull() || opens[i].isNull()) continue;

        IBComm::HistoricalBar bar;
        bar.symbol    = symbol;
        bar.timestamp = dt;
        bar.open      = opens[i].toDouble();
        bar.high      = highs[i].toDouble();
        bar.low       = lows[i].toDouble();
        bar.close     = closes[i].toDouble();
        bar.volume    = vols[i].toDouble();

        if (m_from.isValid() && bar.timestamp < m_from) continue;
        if (m_to.isValid()   && bar.timestamp > m_to)   continue;

        emit barLoaded(bar);
        ++emitted;
    }

    upsertYahooInstrumentMetadata(m_instrumentMetadataDbConnection, symbol, r0);

    qCDebug(lcYahoo) << "YahooFinanceDataSource: parsed" << emitted << "bars for" << symbol;
}

void YahooFinanceDataSource::checkAllDone()
{
    if (m_pendingCount > 0) return;

    if (!m_failedSymbols.isEmpty()) {
        QStringList failed = m_failedSymbols.values();
        failed.sort();
        const QString reason = m_lastError.isEmpty()
            ? QStringLiteral("Yahoo request failed for: %1").arg(failed.join(QStringLiteral(", ")))
            : QStringLiteral("%1 (failed symbols: %2)").arg(m_lastError, failed.join(QStringLiteral(", ")));
        emit loadFailed(reason);
    } else {
        emit loadFinished();
    }
}

} // namespace Backtest
