#include "LiveHistoricalReadAdapter.h"

#include "IBComm/cbrokerdataprovider.h"
#include "IBComm/HistoricalDataRouter.h"
#include "Common/GlobalDef.h"

#include <QDateTime>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(lcLiveHistRead, "pipeline.liveHistoricalRead")

namespace Pipeline {

CBrokerDataProvider* LiveHistoricalReadAdapter::s_broker = nullptr;

LiveHistoricalReadAdapter::LiveHistoricalReadAdapter(QObject* parent)
    : QObject(parent)
{}

void LiveHistoricalReadAdapter::setBrokerDataProvider(CBrokerDataProvider* provider)
{
    s_broker = provider;
}

CBrokerDataProvider* LiveHistoricalReadAdapter::brokerDataProvider()
{
    return s_broker;
}

static QString ibBarSizeFromResolution(const QString& resolution)
{
    const QString r = resolution.trimmed().toUpper();
    if (r == QStringLiteral("DAY1") || r == QStringLiteral("1D") || r.contains(QStringLiteral("DAY")))
        return QStringLiteral("1 day");
    if (r.contains(QStringLiteral("HOUR")) || r == QStringLiteral("1H"))
        return QStringLiteral("1 hour");
    if (r.contains(QStringLiteral("MIN")))
        return QStringLiteral("1 min");
    return QStringLiteral("1 day");
}

static QString ibDurationFromRange(const QDateTime& from, const QDateTime& to)
{
    if (!from.isValid() || !to.isValid())
        return QStringLiteral("30 D");
    const int days = qMax(1, qMin(365 * 5, from.daysTo(to)));
    return QString::number(days) + QStringLiteral(" D");
}

QVector<HistoricalBarSnapshot> LiveHistoricalReadAdapter::getBars(
    const QString& symbol,
    const QString& resolution,
    const QString& dataSourceId,
    const QDateTime& from,
    const QDateTime& to)
{
    Q_UNUSED(dataSourceId);

    QVector<HistoricalBarSnapshot> out;
    if (!s_broker || !s_broker->getClien() || !s_broker->historicalDataRouter()) {
        qCWarning(lcLiveHistRead) << "LiveHistoricalReadAdapter: broker or historical router not set";
        return out;
    }

    reqHistConfigData_t cfg(0, ibBarSizeFromResolution(resolution), ibDurationFromRange(from, to), symbol);
    if (!s_broker->reqestHistoricalData(cfg)) {
        qCWarning(lcLiveHistRead) << "LiveHistoricalReadAdapter: reqestHistoricalData failed for" << symbol;
        return out;
    }

    const int reqId = cfg.id;
    IBComm::HistoricalDataRouter* router = s_broker->historicalDataRouter();

    QVector<IBComm::HistoricalBar> bars;
    QEventLoop loop;
    QMetaObject::Connection conn = QObject::connect(
        router,
        &IBComm::HistoricalDataRouter::barsReceived,
        &loop,
        [&](int id, const QString&, const QVector<IBComm::HistoricalBar>& b) {
            if (id == reqId) {
                bars = b;
                loop.quit();
            }
        },
        Qt::QueuedConnection);

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(60000);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();

    loop.exec();

    QObject::disconnect(conn);

    out.reserve(bars.size());
    for (const auto& bar : bars) {
        HistoricalBarSnapshot s;
        s.timestamp = bar.timestamp;
        s.open = bar.open;
        s.high = bar.high;
        s.low = bar.low;
        s.close = bar.close;
        s.volume = bar.volume;
        out.append(s);
    }

    return out;
}

} // namespace Pipeline
