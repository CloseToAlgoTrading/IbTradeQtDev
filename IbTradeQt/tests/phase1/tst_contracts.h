#ifndef TST_CONTRACTS_H
#define TST_CONTRACTS_H

#include <QtTest>
#include <QJsonDocument>
#include "Pipeline/Contracts.h"

class TestContracts : public QObject
{
    Q_OBJECT

private slots:
    void signalJsonRoundTrip()
    {
        Pipeline::Signal original;
        original.symbol = "AAPL";
        original.confidence = 0.85;
        original.direction = Pipeline::Signal::Buy;
        original.correlationId = "corr-001";
        original.timestamp = QDateTime(QDate(2026, 3, 4), QTime(10, 30, 0), Qt::UTC);
        original.alphaBlockId = "momentum-alpha";

        QJsonObject json = original.toJson();
        Pipeline::Signal restored = Pipeline::Signal::fromJson(json);

        QCOMPARE(restored.symbol, original.symbol);
        QCOMPARE(restored.confidence, original.confidence);
        QCOMPARE(restored.direction, original.direction);
        QCOMPARE(restored.correlationId, original.correlationId);
        QCOMPARE(restored.timestamp, original.timestamp);
        QCOMPARE(restored.alphaBlockId, original.alphaBlockId);
    }

    void signalDirectionEnum()
    {
        QCOMPARE(static_cast<int>(Pipeline::Signal::Buy), 0);
        QCOMPARE(static_cast<int>(Pipeline::Signal::Sell), 1);
        QCOMPARE(static_cast<int>(Pipeline::Signal::Hold), 2);
    }

    void signalDefaultValues()
    {
        Pipeline::Signal s;
        QCOMPARE(s.confidence, 0.0);
        QCOMPARE(s.direction, Pipeline::Signal::Hold);
        QVERIFY(s.symbol.isEmpty());
        QVERIFY(s.correlationId.isEmpty());
    }

    void targetPositionJsonRoundTrip()
    {
        Pipeline::TargetPosition original;
        original.symbol = "MSFT";
        original.targetQuantity = 100.0;
        original.currentQuantity = 50.0;
        original.reason = "Momentum signal";
        original.correlationId = "corr-002";
        original.timestamp = QDateTime(QDate(2026, 3, 4), QTime(11, 0, 0), Qt::UTC);

        QJsonObject json = original.toJson();
        Pipeline::TargetPosition restored = Pipeline::TargetPosition::fromJson(json);

        QCOMPARE(restored.symbol, original.symbol);
        QCOMPARE(restored.targetQuantity, original.targetQuantity);
        QCOMPARE(restored.currentQuantity, original.currentQuantity);
        QCOMPARE(restored.reason, original.reason);
        QCOMPARE(restored.correlationId, original.correlationId);
        QCOMPARE(restored.timestamp, original.timestamp);
    }

    void targetPositionDeltaQuantity()
    {
        Pipeline::TargetPosition tp;
        tp.targetQuantity = 100.0;
        tp.currentQuantity = 60.0;
        QCOMPARE(tp.deltaQuantity(), 40.0);

        tp.targetQuantity = 30.0;
        tp.currentQuantity = 60.0;
        QCOMPARE(tp.deltaQuantity(), -30.0);
    }

    void executionIntentJsonRoundTrip()
    {
        Pipeline::ExecutionIntent original;
        original.symbol = "GOOG";
        original.quantity = -50.0;
        original.orderType = Pipeline::ExecutionIntent::Limit;
        original.limitPrice = 150.25;
        original.riskApproval = "MaxPositionRisk:approved";
        original.correlationId = "corr-003";
        original.timestamp = QDateTime(QDate(2026, 3, 4), QTime(12, 0, 0), Qt::UTC);

        QJsonObject json = original.toJson();
        Pipeline::ExecutionIntent restored = Pipeline::ExecutionIntent::fromJson(json);

        QCOMPARE(restored.symbol, original.symbol);
        QCOMPARE(restored.quantity, original.quantity);
        QCOMPARE(restored.orderType, original.orderType);
        QVERIFY(restored.limitPrice.has_value());
        QCOMPARE(*restored.limitPrice, 150.25);
        QCOMPARE(restored.riskApproval, original.riskApproval);
        QCOMPARE(restored.correlationId, original.correlationId);
        QCOMPARE(restored.timestamp, original.timestamp);
    }

    void executionIntentSideDerivation()
    {
        Pipeline::ExecutionIntent ei;
        ei.quantity = 100.0;
        QCOMPARE(ei.side(), QString("Buy"));
        QCOMPARE(ei.absQuantity(), 100.0);

        ei.quantity = -50.0;
        QCOMPARE(ei.side(), QString("Sell"));
        QCOMPARE(ei.absQuantity(), 50.0);

        ei.quantity = 0.0;
        QCOMPARE(ei.side(), QString("Buy"));
        QCOMPARE(ei.absQuantity(), 0.0);
    }

    void executionIntentNoLimitPrice()
    {
        Pipeline::ExecutionIntent ei;
        ei.symbol = "TSLA";
        ei.quantity = 10.0;
        ei.orderType = Pipeline::ExecutionIntent::Market;

        QJsonObject json = ei.toJson();
        QVERIFY(!json.contains("limitPrice"));

        Pipeline::ExecutionIntent restored = Pipeline::ExecutionIntent::fromJson(json);
        QVERIFY(!restored.limitPrice.has_value());
    }

    void metatypeRegistration()
    {
        QVERIFY(QMetaType::fromName("Pipeline::Signal").isValid()
                || QMetaType::fromType<Pipeline::Signal>().isValid());
        QVERIFY(QMetaType::fromName("Pipeline::TargetPosition").isValid()
                || QMetaType::fromType<Pipeline::TargetPosition>().isValid());
        QVERIFY(QMetaType::fromName("Pipeline::ExecutionIntent").isValid()
                || QMetaType::fromType<Pipeline::ExecutionIntent>().isValid());
    }

    void signalJsonFieldCompleteness()
    {
        Pipeline::Signal s;
        s.symbol = "TEST";
        s.confidence = 0.5;
        s.direction = Pipeline::Signal::Sell;
        s.correlationId = "id-1";
        s.timestamp = QDateTime::currentDateTimeUtc();
        s.alphaBlockId = "alpha-1";

        QJsonObject json = s.toJson();
        QVERIFY(json.contains("symbol"));
        QVERIFY(json.contains("confidence"));
        QVERIFY(json.contains("direction"));
        QVERIFY(json.contains("correlationId"));
        QVERIFY(json.contains("timestamp"));
        QVERIFY(json.contains("alphaBlockId"));
    }
};

#endif // TST_CONTRACTS_H
