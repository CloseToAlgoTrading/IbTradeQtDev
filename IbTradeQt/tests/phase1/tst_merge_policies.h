#ifndef TST_MERGE_POLICIES_H
#define TST_MERGE_POLICIES_H

#include <QtTest>
#include "Pipeline/ISignalMergePolicy.h"
#include "Pipeline/SignalMergePolicies.h"

class TestMergePolicies : public QObject
{
    Q_OBJECT

private:
    Pipeline::Signal makeSignal(const QString& symbol, Pipeline::Signal::Direction dir, double conf, const QString& alphaId)
    {
        Pipeline::Signal s;
        s.symbol = symbol;
        s.direction = dir;
        s.confidence = conf;
        s.alphaBlockId = alphaId;
        s.correlationId = "test-corr";
        s.timestamp = QDateTime::currentDateTimeUtc();
        return s;
    }

private slots:
    // --- WeightedVoteMerge ---
    void weightedVote_emptySignals()
    {
        Pipeline::WeightedVoteMerge merge;
        Pipeline::Signal result = merge.merge({});
        QCOMPARE(result.direction, Pipeline::Signal::Hold);
        QCOMPARE(result.confidence, 0.0);
    }

    void weightedVote_unanimousBuy()
    {
        Pipeline::WeightedVoteMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.8, "alpha1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.6, "alpha2"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Buy);
        QCOMPARE(result.symbol, QString("AAPL"));
        QVERIFY(result.confidence > 0.0);
    }

    void weightedVote_mixedSignals()
    {
        Pipeline::WeightedVoteMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.9, "alpha1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.3, "alpha2"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Buy);
    }

    void weightedVote_sellDominates()
    {
        Pipeline::WeightedVoteMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.2, "alpha1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.8, "alpha2"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.7, "alpha3"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Sell);
    }

    void weightedVote_equalWeightsHold()
    {
        Pipeline::WeightedVoteMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.5, "alpha1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.5, "alpha2"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Hold);
        QCOMPARE(result.confidence, 0.0);
    }

    void weightedVote_metadata()
    {
        Pipeline::WeightedVoteMerge merge;
        QCOMPARE(merge.id(), QString("weighted-vote"));
        QCOMPARE(merge.name(), QString("Weighted Vote Merge"));
    }

    // --- MaxConfidenceMerge ---
    void maxConfidence_emptySignals()
    {
        Pipeline::MaxConfidenceMerge merge;
        Pipeline::Signal result = merge.merge({});
        QCOMPARE(result.confidence, 0.0);
    }

    void maxConfidence_picksHighest()
    {
        Pipeline::MaxConfidenceMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.3, "alpha1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.9, "alpha2"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.5, "alpha3"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Sell);
        QCOMPARE(result.confidence, 0.9);
    }

    void maxConfidence_singleSignal()
    {
        Pipeline::MaxConfidenceMerge merge;
        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.7, "alpha1"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Buy);
        QCOMPARE(result.confidence, 0.7);
    }

    void maxConfidence_metadata()
    {
        Pipeline::MaxConfidenceMerge merge;
        QCOMPARE(merge.id(), QString("max-confidence"));
        QCOMPARE(merge.name(), QString("Max Confidence Merge"));
    }

    // --- ConsensusMerge ---
    void consensus_emptySignals()
    {
        Pipeline::ConsensusMerge merge;
        Pipeline::Signal result = merge.merge({});
        QCOMPARE(result.confidence, 0.0);
    }

    void consensus_buyAboveThreshold()
    {
        Pipeline::ConsensusMerge merge;
        merge.setThreshold(0.6);

        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.7, "a1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.8, "a2"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.6, "a3"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.5, "a4"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Hold, 0.2, "a5"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Buy);
    }

    void consensus_noConsensusHold()
    {
        Pipeline::ConsensusMerge merge;
        merge.setThreshold(0.7);

        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.7, "a1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.5, "a2"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Hold, 0.3, "a3"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Hold);
        QCOMPARE(result.confidence, 0.0);
    }

    void consensus_sellAboveThreshold()
    {
        Pipeline::ConsensusMerge merge;
        merge.setThreshold(0.6);

        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.7, "a1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.8, "a2"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.6, "a3"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.5, "a4"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Sell);
    }

    void consensus_metadata()
    {
        Pipeline::ConsensusMerge merge;
        QCOMPARE(merge.id(), QString("consensus"));
        QCOMPARE(merge.name(), QString("Consensus Merge"));
    }

    void consensus_customThreshold()
    {
        Pipeline::ConsensusMerge merge;
        merge.setThreshold(0.5);

        QVector<Pipeline::Signal> sigList;
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Buy, 0.8, "a1"));
        sigList.append(makeSignal("AAPL", Pipeline::Signal::Sell, 0.6, "a2"));

        Pipeline::Signal result = merge.merge(sigList);
        QCOMPARE(result.direction, Pipeline::Signal::Buy);
    }
};

#endif // TST_MERGE_POLICIES_H
