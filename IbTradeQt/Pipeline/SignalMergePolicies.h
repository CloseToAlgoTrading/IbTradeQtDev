#ifndef PIPELINE_SIGNALMERGEPOLICIES_H
#define PIPELINE_SIGNALMERGEPOLICIES_H

#include "ISignalMergePolicy.h"

namespace Pipeline {

class WeightedVoteMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override;
    QString name() const override;
    Signal merge(const QVector<Signal>& inputSignals) override;
};

class MaxConfidenceMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override;
    QString name() const override;
    Signal merge(const QVector<Signal>& inputSignals) override;
};

class ConsensusMerge : public ISignalMergePolicy {
    Q_OBJECT

public:
    using ISignalMergePolicy::ISignalMergePolicy;

    QString id() const override;
    QString name() const override;
    void setThreshold(double threshold);
    Signal merge(const QVector<Signal>& inputSignals) override;

private:
    double m_threshold = 0.6;
};

} // namespace Pipeline

#endif // PIPELINE_SIGNALMERGEPOLICIES_H
