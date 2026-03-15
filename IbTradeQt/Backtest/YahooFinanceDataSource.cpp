#include "YahooFinanceDataSource.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QTimeZone>
#include <QDebug>

namespace Backtest {

// Yahoo Finance v8 chart API — returns JSON with OHLCV arrays.
// Endpoint: https://query1.finance.yahoo.com/v8/finance/chart/{symbol}
// Query params: interval, period1, period2, events=history
static const QString kYahooBaseUrl =
    "https://query1.finance.yahoo.com/v8/finance/chart/%1";

static QString intervalFor(BarResolution r) {
    switch (r) {
    case BarResolution::Min1:  return "1m";
    case BarResolution::Min5:  return "5m";
    case BarResolution::Min15: return "15m";
    case BarResolution::Min30: return "30m";
    case BarResolution::Hour1: return "1h";
    case BarResolution::Day1:  return "1d";
    default:                   return "1d";
    }
}

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

void YahooFinanceDataSource::requestBars(const QStringList& symbols,
                                         const QDateTime& from,
                                         const QDateTime& to,
                                         BarResolution resolution)
{
    if (symbols.isEmpty()) {
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

    m_requestedSymbols = symbols;
    m_from             = from;
    m_to               = to;
    m_pendingCount     = symbols.size();
    m_failed           = false;
    m_lastError.clear();

    const QString interval = intervalFor(resolution);
    const qint64  period1  = from.isValid() ? from.toSecsSinceEpoch() : 0;
    const qint64  period2  = to.isValid()   ? to.toSecsSinceEpoch()
                                            : QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    for (const QString& symbol : symbols) {
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

        qDebug() << "YahooFinanceDataSource: GET" << url.toString();
        m_nam->get(req);
    }
}

void YahooFinanceDataSource::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (m_failed) {
        --m_pendingCount;
        checkAllDone();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString symbol = reply->request().attribute(QNetworkRequest::User).toString();
        m_lastError = QString("Network error for %1: %2").arg(symbol, reply->errorString());
        qWarning() << "YahooFinanceDataSource:" << m_lastError;
        m_failed = true;
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
        qWarning() << "YahooFinanceDataSource: JSON parse error for" << symbol
                   << parseErr.errorString();
        return;
    }

    // Navigate: chart -> result[0] -> { timestamp[], indicators.quote[0].{open,high,low,close,volume} }
    const QJsonObject root   = doc.object();
    const QJsonObject chart  = root["chart"].toObject();
    const QJsonArray  result = chart["result"].toArray();

    if (result.isEmpty()) {
        const QString errMsg = chart["error"].toObject()["description"].toString("unknown");
        qWarning() << "YahooFinanceDataSource: no result for" << symbol << "—" << errMsg;
        return;
    }

    const QJsonObject r0         = result[0].toObject();
    const QJsonArray  timestamps = r0["timestamp"].toArray();
    const QJsonObject indicators = r0["indicators"].toObject();
    const QJsonArray  quoteArr   = indicators["quote"].toArray();

    if (quoteArr.isEmpty() || timestamps.isEmpty()) {
        qWarning() << "YahooFinanceDataSource: empty quote data for" << symbol;
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

    qDebug() << "YahooFinanceDataSource: parsed" << emitted << "bars for" << symbol;
}

void YahooFinanceDataSource::checkAllDone()
{
    if (m_pendingCount > 0) return;

    if (m_failed) {
        emit loadFailed(m_lastError);
    } else {
        emit loadFinished();
    }
}

} // namespace Backtest
