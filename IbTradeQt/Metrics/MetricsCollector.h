#ifndef METRICS_METRICSCOLLECTOR_H
#define METRICS_METRICSCOLLECTOR_H

#include <QString>
#include <QMap>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>

namespace Metrics {

class Histogram {
public:
    void record(double value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values.push_back(value);
        m_sum += value;
        m_count++;
        if (value < m_min) m_min = value;
        if (value > m_max) m_max = value;
    }

    double average() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count > 0 ? m_sum / m_count : 0.0;
    }

    double percentile(int p) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_values.empty()) return 0.0;
        auto sorted = m_values;
        std::sort(sorted.begin(), sorted.end());
        size_t index = std::min(
            static_cast<size_t>((sorted.size() * p) / 100),
            sorted.size() - 1);
        return sorted[index];
    }

    double min() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_min;
    }

    double max() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_max;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values.clear();
        m_sum = 0.0;
        m_count = 0;
        m_min = std::numeric_limits<double>::max();
        m_max = std::numeric_limits<double>::lowest();
    }

private:
    mutable std::mutex m_mutex;
    std::vector<double> m_values;
    double m_sum = 0.0;
    size_t m_count = 0;
    double m_min = std::numeric_limits<double>::max();
    double m_max = std::numeric_limits<double>::lowest();
};

class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector collector;
        return collector;
    }

    void recordOrderPlaced(const QString& symbol) {
        Q_UNUSED(symbol)
        m_ordersPlaced.fetch_add(1, std::memory_order_relaxed);
    }

    void recordOrderFilled(const QString& symbol) {
        Q_UNUSED(symbol)
        m_ordersFilled.fetch_add(1, std::memory_order_relaxed);
    }

    void recordOrderRejected(const QString& symbol) {
        Q_UNUSED(symbol)
        m_ordersRejected.fetch_add(1, std::memory_order_relaxed);
    }

    void recordOrderLatencyUs(double microseconds) {
        m_latencyHistogram.record(microseconds);
    }

    void recordPipelineLatencyUs(double microseconds) {
        m_pipelineLatencyHistogram.record(microseconds);
    }

    void recordTickProcessed() {
        m_ticksProcessed.fetch_add(1, std::memory_order_relaxed);
    }

    void recordSignalGenerated() {
        m_signalsGenerated.fetch_add(1, std::memory_order_relaxed);
    }

    void recordRiskRejection() {
        m_riskRejections.fetch_add(1, std::memory_order_relaxed);
    }

    void recordStrategyPnL(const QString& strategyName, double pnl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_strategyPnL[strategyName] = pnl;
    }

    struct Snapshot {
        uint64_t ordersPlaced;
        uint64_t ordersFilled;
        uint64_t ordersRejected;
        uint64_t ticksProcessed;
        uint64_t signalsGenerated;
        uint64_t riskRejections;
        double avgOrderLatencyUs;
        double p50OrderLatencyUs;
        double p99OrderLatencyUs;
        double avgPipelineLatencyUs;
        double p99PipelineLatencyUs;
        QMap<QString, double> strategyPnL;
    };

    Snapshot snapshot() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return {
            m_ordersPlaced.load(std::memory_order_relaxed),
            m_ordersFilled.load(std::memory_order_relaxed),
            m_ordersRejected.load(std::memory_order_relaxed),
            m_ticksProcessed.load(std::memory_order_relaxed),
            m_signalsGenerated.load(std::memory_order_relaxed),
            m_riskRejections.load(std::memory_order_relaxed),
            m_latencyHistogram.average(),
            m_latencyHistogram.percentile(50),
            m_latencyHistogram.percentile(99),
            m_pipelineLatencyHistogram.average(),
            m_pipelineLatencyHistogram.percentile(99),
            m_strategyPnL
        };
    }

    void reset() {
        m_ordersPlaced = 0;
        m_ordersFilled = 0;
        m_ordersRejected = 0;
        m_ticksProcessed = 0;
        m_signalsGenerated = 0;
        m_riskRejections = 0;
        m_latencyHistogram.reset();
        m_pipelineLatencyHistogram.reset();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_strategyPnL.clear();
    }

private:
    MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    std::atomic<uint64_t> m_ordersPlaced{0};
    std::atomic<uint64_t> m_ordersFilled{0};
    std::atomic<uint64_t> m_ordersRejected{0};
    std::atomic<uint64_t> m_ticksProcessed{0};
    std::atomic<uint64_t> m_signalsGenerated{0};
    std::atomic<uint64_t> m_riskRejections{0};

    Histogram m_latencyHistogram;
    Histogram m_pipelineLatencyHistogram;

    mutable std::mutex m_mutex;
    QMap<QString, double> m_strategyPnL;
};

} // namespace Metrics

#endif // METRICS_METRICSCOLLECTOR_H
