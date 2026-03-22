#include "SignalMergePolicies.h"

#include <QDateTime>

namespace Pipeline {

QString WeightedVoteMerge::id() const { return QStringLiteral("weighted-vote"); }
QString WeightedVoteMerge::name() const { return QStringLiteral("Weighted Vote Merge"); }

Signal WeightedVoteMerge::merge(const QVector<Signal>& inputSignals)
{
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

    merged.alphaBlockId = QStringLiteral("merged");
    return merged;
}

QString MaxConfidenceMerge::id() const { return QStringLiteral("max-confidence"); }
QString MaxConfidenceMerge::name() const { return QStringLiteral("Max Confidence Merge"); }

Signal MaxConfidenceMerge::merge(const QVector<Signal>& inputSignals)
{
    if (inputSignals.isEmpty()) return {};

    const Signal* best = &inputSignals.first();
    for (const auto& s : inputSignals) {
        if (s.confidence > best->confidence) {
            best = &s;
        }
    }

    Signal merged = *best;
    merged.alphaBlockId = QStringLiteral("merged-max-") + best->alphaBlockId;
    return merged;
}

QString ConsensusMerge::id() const { return QStringLiteral("consensus"); }
QString ConsensusMerge::name() const { return QStringLiteral("Consensus Merge"); }

void ConsensusMerge::setThreshold(double threshold) { m_threshold = threshold; }

Signal ConsensusMerge::merge(const QVector<Signal>& inputSignals)
{
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
    merged.alphaBlockId = QStringLiteral("merged-consensus");

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

} // namespace Pipeline
