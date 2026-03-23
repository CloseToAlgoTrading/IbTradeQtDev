#ifndef TST_SEMANTIC_MAPPING_H
#define TST_SEMANTIC_MAPPING_H

#include <QtTest>
#include <QDateTime>
#include "Pipeline/SemanticModelDataMapper.h"
#include "Pipeline/SemanticPipelineChain.h"
#include "Strategies/Generic/UnifiedModelData.h"

class TestSemanticMapping : public QObject
{
    Q_OBJECT
private slots:
    void modelData_to_signals_carries_suggested_quantity()
    {
        Pipeline::ModelDataList md = createDataList();
        md->append(UnifiedModelData(QStringLiteral("AAA"), DIRECTION_UP, 0.5, 10.0, 0.0));

        const QVector<Pipeline::Signal> sigs =
            Pipeline::SemanticMapping::signalsFromModelData(md, QStringLiteral("c1"), QStringLiteral("alpha"));
        QCOMPARE(sigs.size(), 1);
        QCOMPARE(sigs[0].symbol, QStringLiteral("AAA"));
        QCOMPARE(sigs[0].suggestedQuantity, 10.0);
    }

    void signals_round_trip_to_model_data()
    {
        Pipeline::Signal s;
        s.symbol = QStringLiteral("X");
        s.direction = Pipeline::Signal::Buy;
        s.suggestedQuantity = 50.0;
        s.correlationId = QStringLiteral("c");

        Pipeline::ModelDataList md = Pipeline::SemanticMapping::modelDataFromSignals({s});
        QVERIFY(md && !md->isEmpty());
        QCOMPARE(md->first().amount, 50.0);
    }

    void merge_model_data_with_tick_signals_last_tick_wins_per_symbol()
    {
        Pipeline::ModelDataList base = createDataList();
        base->append(UnifiedModelData(QStringLiteral("AAA"), DIRECTION_UP, 0.5, 10.0, 0.0));

        Pipeline::Signal tick;
        tick.symbol = QStringLiteral("AAA");
        tick.direction = Pipeline::Signal::Sell;
        tick.suggestedQuantity = 3.0;

        const Pipeline::ModelDataList merged = Pipeline::mergeModelDataWithTickSignals(
            base,
            {tick},
            true,
            QStringLiteral("cid"),
            QStringLiteral("alpha"));

        const QVector<Pipeline::Signal> out =
            Pipeline::SemanticMapping::signalsFromModelData(merged, QStringLiteral("cid"), QStringLiteral("alpha"));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].direction, Pipeline::Signal::Sell);
    }

    void targetPositions_downRow_is_delta_not_absolute_short()
    {
        Pipeline::ModelDataList md = createDataList();
        md->append(UnifiedModelData(QStringLiteral("Z"), DIRECTION_DOWN, 1.0, 220.0, 0.0));

        QMap<QString, double> pos;
        pos[QStringLiteral("Z")] = 220.0;

        const QVector<Pipeline::ExecutionIntent> intents =
            Pipeline::SemanticMapping::executionIntentsFromModelData(
                md, pos, QStringLiteral("c1"), QDateTime::currentDateTimeUtc());
        QCOMPARE(intents.size(), 1);
        QCOMPARE(intents[0].quantity, -220.0);
    }
};

#endif // TST_SEMANTIC_MAPPING_H
