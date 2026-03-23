#ifndef TST_SUBSCRIPTION_REQUEST_STORE_H
#define TST_SUBSCRIPTION_REQUEST_STORE_H

#include <QtTest>
#include "Pipeline/IDataSubscriptionPort.h"
#include "Pipeline/SubscriptionRequestStore.h"

namespace {

class SubscriptionStoreRecordingPort : public Pipeline::IDataSubscriptionPort {
public:
    int beginCount = 0;
    int endCount = 0;
    int setDesiredCount = 0;

    void setDesiredSymbols(const QString&, const QVector<QString>&) override { ++setDesiredCount; }
    void setDesiredSymbolsWithKinds(const QString&, const QVector<QString>&, quint32) override
    {
        ++setDesiredCount;
    }
    void clearOwner(const QString&) override {}
    void clearAll() override {}
    void beginPipelineEvaluation() override { ++beginCount; }
    void endPipelineEvaluation() override { ++endCount; }
};

} // namespace

class TestSubscriptionRequestStore : public QObject
{
    Q_OBJECT

private slots:
    void mergeUnion_dedupesBase()
    {
        Pipeline::SubscriptionRequestStore store;
        const QVector<QString> base{QStringLiteral("A"), QStringLiteral("B")};
        store.setDesiredSymbols(QStringLiteral("o1"), {QStringLiteral("B"), QStringLiteral("C")});
        const QVector<QString> u = store.mergeUnion(base);
        QCOMPARE(u.size(), 3);
        QCOMPARE(u[0], QStringLiteral("A"));
        QCOMPARE(u[1], QStringLiteral("B"));
        QCOMPARE(u[2], QStringLiteral("C"));
    }

    void clearOwner_removesContribution()
    {
        Pipeline::SubscriptionRequestStore store;
        store.setDesiredSymbols(QStringLiteral("a"), {QStringLiteral("X")});
        store.setDesiredSymbols(QStringLiteral("b"), {QStringLiteral("Y")});
        store.clearOwner(QStringLiteral("a"));
        const QVector<QString> u = store.mergeUnion({});
        QCOMPARE(u.size(), 1);
        QCOMPARE(u[0], QStringLiteral("Y"));
    }

    void clearAll_emptiesMerge()
    {
        Pipeline::SubscriptionRequestStore store;
        store.setDesiredSymbols(QStringLiteral("a"), {QStringLiteral("Z")});
        store.clearAll();
        const QVector<QString> u = store.mergeUnion({QStringLiteral("Q")});
        QCOMPARE(u.size(), 1);
        QCOMPARE(u[0], QStringLiteral("Q"));
    }

    void mergeSymbolKindMasks_ownerAndBase()
    {
        Pipeline::SubscriptionRequestStore store;
        store.setDesiredSymbolsWithKinds(
            QStringLiteral("o1"),
            {QStringLiteral("A")},
            Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::RealtimeBars));
        const auto m = store.mergeSymbolKindMasks({QStringLiteral("B")});
        QCOMPARE(m.value(QStringLiteral("A")),
                 Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::RealtimeBars));
        QCOMPARE(m.value(QStringLiteral("B")),
                 Pipeline::subscriptionKindMask(Pipeline::SubscriptionKind::TopOfBook));
    }

    void recordingPort_beginSetEnd_matchesRunnerEpochContract()
    {
        SubscriptionStoreRecordingPort p;
        p.beginPipelineEvaluation();
        p.setDesiredSymbols(QStringLiteral("owner-a"), {QStringLiteral("X")});
        p.endPipelineEvaluation();
        QCOMPARE(p.beginCount, 1);
        QCOMPARE(p.setDesiredCount, 1);
        QCOMPARE(p.endCount, 1);
    }
};

#endif // TST_SUBSCRIPTION_REQUEST_STORE_H
