#ifndef TST_PIPELINE_CONFIG_MUTATIONS_H
#define TST_PIPELINE_CONFIG_MUTATIONS_H

#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>

#include "Pipeline/PipelineConfigMutations.h"

class TestPipelineConfigMutations : public QObject
{
    Q_OBJECT

private slots:
    void addBlock_appendsToAlphaArray()
    {
        QJsonObject pipeline;
        QJsonObject defCfg;
        defCfg[QStringLiteral("k")] = 1;
        QVERIFY(Pipeline::addBlockToPipeline(
            pipeline, QStringLiteral("Alpha"), QStringLiteral("momentum-alpha"), defCfg));

        QJsonArray alphas = pipeline[QStringLiteral("alphas")].toArray();
        QCOMPARE(alphas.size(), 1);
        QCOMPARE(alphas[0].toObject()[QStringLiteral("blockId")].toString(),
                 QStringLiteral("momentum-alpha"));
    }

    void addBlock_setsSingleRebalanceSlot()
    {
        QJsonObject pipeline;
        QJsonObject defCfg;
        QVERIFY(Pipeline::addBlockToPipeline(
            pipeline, QStringLiteral("Rebalance"), QStringLiteral("simple-rebalance"), defCfg));

        QJsonObject blk = pipeline[QStringLiteral("rebalance")].toObject();
        QCOMPARE(blk[QStringLiteral("blockId")].toString(), QStringLiteral("simple-rebalance"));
    }

    void removeBlock_removesFromArray()
    {
        QJsonObject pipeline;
        QJsonObject b0;
        b0[QStringLiteral("blockId")] = QStringLiteral("a");
        b0[QStringLiteral("config")]  = QJsonObject();
        QJsonObject b1;
        b1[QStringLiteral("blockId")] = QStringLiteral("b");
        b1[QStringLiteral("config")]  = QJsonObject();
        QJsonArray arr;
        arr.append(b0);
        arr.append(b1);
        pipeline[QStringLiteral("alphas")] = arr;

        QVERIFY(Pipeline::removeBlockFromPipeline(pipeline, QStringLiteral("Alpha"), 0));
        QJsonArray out = pipeline[QStringLiteral("alphas")].toArray();
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].toObject()[QStringLiteral("blockId")].toString(), QStringLiteral("b"));
    }

    void removeBlock_clearsSingleSlot()
    {
        QJsonObject pipeline;
        QJsonObject blk;
        blk[QStringLiteral("blockId")] = QStringLiteral("x");
        blk[QStringLiteral("config")]  = QJsonObject();
        pipeline[QStringLiteral("rebalance")] = blk;

        QVERIFY(Pipeline::removeBlockFromPipeline(pipeline, QStringLiteral("Rebalance"), 0));
        QVERIFY(!pipeline.contains(QStringLiteral("rebalance")));
    }

    void removeBlock_invalidIndex_fails()
    {
        QJsonObject pipeline;
        QJsonArray arr;
        QJsonObject b;
        b[QStringLiteral("blockId")] = QStringLiteral("a");
        b[QStringLiteral("config")]  = QJsonObject();
        arr.append(b);
        pipeline[QStringLiteral("alphas")] = arr;

        QVERIFY(!Pipeline::removeBlockFromPipeline(pipeline, QStringLiteral("Alpha"), 99));
    }
};

#endif // TST_PIPELINE_CONFIG_MUTATIONS_H
