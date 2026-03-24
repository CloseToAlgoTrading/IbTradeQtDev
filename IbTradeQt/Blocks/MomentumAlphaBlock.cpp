#include "MomentumAlphaBlock.h"

#include "../Pipeline/BlockSubscriptionUtils.h"
#include "../Pipeline/IHistoricalRead.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Pipeline/SemanticModelDataMapper.h"
#include <QDateTime>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

namespace Blocks {

MomentumAlphaBlock::MomentumAlphaBlock(QObject* parent)
    : IAlphaBlock(parent)
{}

QString MomentumAlphaBlock::id() const { return QStringLiteral("momentum-alpha"); }
QString MomentumAlphaBlock::name() const { return QStringLiteral("Momentum Alpha"); }

QString MomentumAlphaBlock::description() const
{
    return QStringLiteral(
        "Ranks the selection universe by momentum vs a threshold, then scores the top N symbols for "
        "downstream sizing (period, threshold, topN)");
}

QJsonObject MomentumAlphaBlock::config() const
{
    QJsonObject cfg;
    cfg[QStringLiteral("period")] = m_period;
    cfg[QStringLiteral("threshold")] = m_threshold;
    cfg[QStringLiteral("topN")] = m_topN;
    cfg[QStringLiteral("positionSize")] = m_positionSize;
    cfg[QStringLiteral("resolution")] = m_resolution;
    cfg[QStringLiteral("dataSourceId")] = m_dataSourceId;
    cfg[QStringLiteral("lookbackYears")] = m_lookbackYears;
    return cfg;
}

void MomentumAlphaBlock::setConfig(const QJsonObject& config)
{
    m_period = config.value(QStringLiteral("period")).toInt(20);
    m_threshold = config.value(QStringLiteral("threshold")).toDouble(0.02);
    m_topN = config.value(QStringLiteral("topN")).toInt(3);
    m_positionSize = config.value(QStringLiteral("positionSize")).toDouble(100.0);
    m_resolution = config.value(QStringLiteral("resolution")).toString(QStringLiteral("Day1"));
    m_dataSourceId = config.value(QStringLiteral("dataSourceId")).toString(QStringLiteral("yahoo"));
    m_lookbackYears = config.value(QStringLiteral("lookbackYears")).toInt(1);
}

void MomentumAlphaBlock::initialize() {}
void MomentumAlphaBlock::shutdown() {}

Pipeline::ModelDataList MomentumAlphaBlock::processSemantic(
    const Pipeline::ModelDataList& in,
    const QString& correlationId)
{
    Q_UNUSED(correlationId);
    if (!in || in->isEmpty())
        return in;
    if (!runtimeContext() || !runtimeContext()->historical) {
        return in;
    }

    const QDateTime to =
        m_clock ? m_clock->now().toUTC() : QDateTime::currentDateTimeUtc();
    const QDateTime from = to.addYears(-m_lookbackYears);

    const int period = qMax(1, m_period);

    QMap<QString, double> momentumBySymbol;
    for (const auto& row : *in) {
        const QString sym = row.symbol.trimmed().toUpper();
        if (sym.isEmpty())
            continue;
        QVector<Pipeline::HistoricalBarSnapshot> bars =
            runtimeContext()->historical->getBars(sym, m_resolution, m_dataSourceId, from, to);
        if (bars.size() <= period)
            continue;
        const double baseClose = bars[bars.size() - 1 - period].close;
        const double lastClose = bars.last().close;
        if (baseClose <= 0.0)
            continue;
        momentumBySymbol.insert(sym, (lastClose - baseClose) / baseClose);
    }
    if (momentumBySymbol.isEmpty()) {
        if (runtimeContext()->subscription) {
            const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("alpha:"), id());
            runtimeContext()->subscription->clearOwner(oid);
        }
        return createDataList();
    }

    QList<QPair<QString, double>> ranked;
    for (auto it = momentumBySymbol.constBegin(); it != momentumBySymbol.constEnd(); ++it)
        ranked.append(qMakePair(it.key(), it.value()));
    std::sort(ranked.begin(), ranked.end(),
              [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                  return a.second > b.second;
              });

    Pipeline::ModelDataList out = createDataList();
    const int n = qMin(m_topN, ranked.size());
    for (int i = 0; i < n; ++i) {
        out->append(UnifiedModelData(
            ranked[i].first,
            DIRECTION_UP,
            std::min(std::abs(ranked[i].second) / qMax(m_threshold, 1e-9), 1.0),
            m_positionSize,
            0.0));
    }
    if (runtimeContext()->subscription) {
        const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("alpha:"), id());
        QVector<QString> active;
        active.reserve(n);
        for (int i = 0; i < n; ++i)
            active.append(ranked[i].first);
        runtimeContext()->subscription->setDesiredSymbols(oid, active);
    }
    return out;
}

void MomentumAlphaBlock::onTick(const Pipeline::MarketTick& tick)
{
    Q_UNUSED(tick);
}

} // namespace Blocks
