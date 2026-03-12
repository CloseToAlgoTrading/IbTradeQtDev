#ifndef BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H
#define BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QUuid>
#include <cmath>
#include <numeric>
#include "../Pipeline/IAlphaBlock.h"
#include "../Pipeline/BlockRegistry.h"
#include "../IBComm/HistoricalDataRouter.h"

namespace Blocks {

class MovingAverageCrossoverAlphaBlock : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit MovingAverageCrossoverAlphaBlock(QObject* parent = nullptr)
        : IAlphaBlock(parent) {}

    QString id() const override { return "ma-crossover-alpha"; }
    QString name() const override { return "Moving Average Crossover"; }
    QString description() const override {
        return "Generates signals when a fast MA crosses above/below a slow MA";
    }

    QJsonObject config() const override {
        QJsonObject cfg;
        cfg["fastPeriod"] = m_fastPeriod;
        cfg["slowPeriod"] = m_slowPeriod;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_fastPeriod = config.value("fastPeriod").toInt(10);
        m_slowPeriod = config.value("slowPeriod").toInt(30);
    }

    void initialize() override { m_closes.clear(); m_prevFastAboveSlow.clear(); }
    void shutdown() override { m_closes.clear(); m_prevFastAboveSlow.clear(); }

public slots:
    void onTick(const IBComm::MarketTick& tick) override {
        auto& history = m_closes[tick.symbol];
        history.append(tick.mid());
        if (history.size() > m_slowPeriod + 1)
            history.removeFirst();
        checkCrossover(tick.symbol, tick.timestamp);
    }

    void onHistoricalBars(const QVector<IBComm::HistoricalBar>& bars) {
        if (bars.isEmpty()) return;
        const QString& symbol = bars.first().symbol;
        auto& history = m_closes[symbol];
        for (const auto& bar : bars) {
            history.append(bar.close);
        }
        while (history.size() > m_slowPeriod + 1)
            history.removeFirst();
    }

signals:
    void signalGenerated(const Pipeline::Signal& signal);

private:
    void checkCrossover(const QString& symbol, const QDateTime& ts) {
        const auto& history = m_closes[symbol];
        if (history.size() < m_slowPeriod) return;

        double fastMA = sma(history, m_fastPeriod);
        double slowMA = sma(history, m_slowPeriod);
        bool fastAbove = fastMA > slowMA;
        bool hadPrev = m_prevFastAboveSlow.contains(symbol);
        bool prevAbove = m_prevFastAboveSlow.value(symbol, false);
        m_prevFastAboveSlow[symbol] = fastAbove;

        if (!hadPrev) return;

        if (fastAbove && !prevAbove) {
            emitSignal(symbol, Pipeline::Signal::Buy, fastMA, slowMA, ts);
        } else if (!fastAbove && prevAbove) {
            emitSignal(symbol, Pipeline::Signal::Sell, fastMA, slowMA, ts);
        }
    }

    void emitSignal(const QString& symbol, Pipeline::Signal::Direction dir,
                    double fastMA, double slowMA, const QDateTime& ts) {
        Pipeline::Signal sig;
        sig.symbol = symbol;
        sig.direction = dir;
        sig.confidence = std::min(std::abs(fastMA - slowMA) / slowMA * 100.0, 1.0);
        sig.correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        sig.timestamp = ts;
        sig.alphaBlockId = id();
        emit Pipeline::IAlphaBlock::signalGenerated(sig);
    }

    static double sma(const QVector<double>& data, int period) {
        if (data.size() < period) return 0.0;
        double sum = 0.0;
        for (int i = data.size() - period; i < data.size(); ++i)
            sum += data[i];
        return sum / period;
    }

    int m_fastPeriod = 10;
    int m_slowPeriod = 30;
    QMap<QString, QVector<double>> m_closes;
    QMap<QString, bool> m_prevFastAboveSlow;
};

} // namespace Blocks

#endif // BLOCKS_MOVINGAVERAGECROSSOVERALPHABLOCK_H
