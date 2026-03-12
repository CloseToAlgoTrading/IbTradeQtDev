#ifndef PIPELINE_ISIGNALMERGEPOLICY_H
#define PIPELINE_ISIGNALMERGEPOLICY_H

#include <QObject>
#include <QVector>
#include "Contracts.h"

namespace Pipeline {

class ISignalMergePolicy : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~ISignalMergePolicy() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;

    virtual Signal merge(const QVector<Signal>& inputSignals) = 0;
};

class WeightedVoteMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override { return "weighted-vote"; }
    QString name() const override { return "Weighted Vote Merge"; }

    Signal merge(const QVector<Signal>& inputSignals) override {
        if (inputSignals.isEmpty()) return {};

        double buyWeight = 0.0;
        double sellWeight = 0.0;
        double totalConfidence = 0.0;
        QString symbol = inputSignals.first().symbol;
        QString correlationId = inputSignals.first().correlationId;

        for (const auto& s : inputSignals) {
            double weight = s.confidence;
            if (s.direction == Signal::Buy) buyWeight += weight;
            else if (s.direction == Signal::Sell) sellWeight += weight;
            totalConfidence += s.confidence;
        }

        Signal merged;
        merged.symbol = symbol;
        merged.correlationId = correlationId;
        merged.timestamp = QDateTime::currentDateTime();

        if (buyWeight > sellWeight) {
            merged.direction = Signal::Buy;
            merged.confidence = buyWeight / totalConfidence;
        } else if (sellWeight > buyWeight) {
            merged.direction = Signal::Sell;
            merged.confidence = sellWeight / totalConfidence;
        } else {
            merged.direction = Signal::Hold;
            merged.confidence = 0.0;
        }

        merged.alphaBlockId = "merged";
        return merged;
    }
};

class MaxConfidenceMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override { return "max-confidence"; }
    QString name() const override { return "Max Confidence Merge"; }

    Signal merge(const QVector<Signal>& inputSignals) override {
        if (inputSignals.isEmpty()) return {};

        const Signal* best = &inputSignals.first();
        for (const auto& s : inputSignals) {
            if (s.confidence > best->confidence) {
                best = &s;
            }
        }

        Signal merged = *best;
        merged.alphaBlockId = "merged-max-" + best->alphaBlockId;
        return merged;
    }
};

class ConsensusMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override { return "consensus"; }
    QString name() const override { return "Consensus Merge"; }

    void setThreshold(double threshold) { m_threshold = threshold; }

    Signal merge(const QVector<Signal>& inputSignals) override {
        if (inputSignals.isEmpty()) return {};

        int buyCount = 0;
        int sellCount = 0;
        double totalConfidence = 0.0;

        for (const auto& s : inputSignals) {
            if (s.direction == Signal::Buy) buyCount++;
            else if (s.direction == Signal::Sell) sellCount++;
            totalConfidence += s.confidence;
        }

        int total = inputSignals.size();
        Signal merged;
        merged.symbol = inputSignals.first().symbol;
        merged.correlationId = inputSignals.first().correlationId;
        merged.timestamp = QDateTime::currentDateTime();
        merged.alphaBlockId = "merged-consensus";

        double buyRatio = static_cast<double>(buyCount) / total;
        double sellRatio = static_cast<double>(sellCount) / total;

        if (buyRatio >= m_threshold) {
            merged.direction = Signal::Buy;
            merged.confidence = totalConfidence / total;
        } else if (sellRatio >= m_threshold) {
            merged.direction = Signal::Sell;
            merged.confidence = totalConfidence / total;
        } else {
            merged.direction = Signal::Hold;
            merged.confidence = 0.0;
        }

        return merged;
    }

private:
    double m_threshold = 0.6;
};

} // namespace Pipeline

#endif // PIPELINE_ISIGNALMERGEPOLICY_H
