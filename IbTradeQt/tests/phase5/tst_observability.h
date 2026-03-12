#ifndef TST_OBSERVABILITY_H
#define TST_OBSERVABILITY_H

#include <QtTest>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include "Logging/StructuredLogger.h"
#include "Metrics/MetricsCollector.h"

class TestObservability : public QObject
{
    Q_OBJECT

private slots:

    void init()
    {
        Logging::StructuredLogger::instance().reset();
        Logging::StructuredLogger::clearCorrelationId();
        Metrics::MetricsCollector::instance().reset();
    }

    // --- StructuredLogger tests ---

    void logger_infoLogsEntry()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        LOG_INFO("TestComponent", "Hello world");

        QCOMPARE(captured.size(), 1);
        QCOMPARE(captured[0].level, QString("INFO"));
        QCOMPARE(captured[0].component, QString("TestComponent"));
        QCOMPARE(captured[0].message, QString("Hello world"));
        QVERIFY(captured[0].timestamp.isValid());
    }

    void logger_warnLogsEntry()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        LOG_WARN("RiskBlock", "Position too large");

        QCOMPARE(captured.size(), 1);
        QCOMPARE(captured[0].level, QString("WARN"));
    }

    void logger_errorLogsEntry()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        LOG_ERROR("Execution", "Order failed");

        QCOMPARE(captured.size(), 1);
        QCOMPARE(captured[0].level, QString("ERROR"));
    }

    void logger_contextIncluded()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        QVariantMap ctx;
        ctx["symbol"] = "AAPL";
        ctx["quantity"] = 100;
        ctx["price"] = 150.25;
        LOG_INFO("OrderExec", "Placing order", ctx);

        QCOMPARE(captured[0].context["symbol"].toString(), QString("AAPL"));
        QCOMPARE(captured[0].context["quantity"].toInt(), 100);
        QCOMPARE(captured[0].context["price"].toDouble(), 150.25);
    }

    void logger_correlationIdDefault()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        LOG_INFO("Test", "No correlation");

        QCOMPARE(captured[0].correlationId, QString("none"));
    }

    void logger_correlationIdSet()
    {
        QVector<Logging::LogEntry> captured;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry& e) { captured.append(e); });

        Logging::StructuredLogger::setCorrelationId("abc-123-def");
        LOG_INFO("Test", "With correlation");

        QCOMPARE(captured[0].correlationId, QString("abc-123-def"));
    }

    void logger_correlationIdClear()
    {
        Logging::StructuredLogger::setCorrelationId("test-id");
        QCOMPARE(Logging::StructuredLogger::correlationId(), QString("test-id"));

        Logging::StructuredLogger::clearCorrelationId();
        QCOMPARE(Logging::StructuredLogger::correlationId(), QString("none"));
    }

    void logger_multipleCallbacks()
    {
        int count1 = 0, count2 = 0;
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry&) { count1++; });
        Logging::StructuredLogger::instance().addCallback(
            [&](const Logging::LogEntry&) { count2++; });

        LOG_INFO("Test", "Multi");

        QCOMPARE(count1, 1);
        QCOMPARE(count2, 1);
    }

    void logger_entryToJson()
    {
        Logging::LogEntry entry;
        entry.timestamp = QDateTime(QDate(2026, 3, 4), QTime(12, 0, 0), Qt::UTC);
        entry.correlationId = "corr-1";
        entry.component = "Alpha";
        entry.level = "INFO";
        entry.message = "Signal generated";
        entry.context = {{"symbol", "AAPL"}, {"confidence", 0.85}};

        QJsonObject json = entry.toJson();
        QCOMPARE(json["correlationId"].toString(), QString("corr-1"));
        QCOMPARE(json["component"].toString(), QString("Alpha"));
        QCOMPARE(json["level"].toString(), QString("INFO"));
        QCOMPARE(json["message"].toString(), QString("Signal generated"));
        QVERIFY(json.contains("context"));
    }

    void logger_entryToJsonLine()
    {
        Logging::LogEntry entry;
        entry.timestamp = QDateTime(QDate(2026, 1, 1), QTime(0, 0, 0), Qt::UTC);
        entry.correlationId = "x";
        entry.component = "C";
        entry.level = "WARN";
        entry.message = "msg";

        QString line = entry.toJsonLine();
        QVERIFY(!line.contains('\n'));

        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object()["level"].toString(), QString("WARN"));
    }

    void logger_fileOutput()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString path = tmpFile.fileName();
        tmpFile.close();

        QVERIFY(Logging::StructuredLogger::instance().openLogFile(path));
        LOG_INFO("FileTest", "Line one");
        LOG_WARN("FileTest", "Line two");
        Logging::StructuredLogger::instance().closeLogFile();

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray data = f.readAll();
        QStringList lines = QString::fromUtf8(data).trimmed().split('\n');
        QCOMPARE(lines.size(), 2);

        QJsonDocument doc1 = QJsonDocument::fromJson(lines[0].toUtf8());
        QCOMPARE(doc1.object()["message"].toString(), QString("Line one"));
        QJsonDocument doc2 = QJsonDocument::fromJson(lines[1].toUtf8());
        QCOMPARE(doc2.object()["level"].toString(), QString("WARN"));
    }

    void logger_entryCount()
    {
        QCOMPARE(Logging::StructuredLogger::instance().entryCount(), 0);
        LOG_INFO("Test", "a");
        LOG_INFO("Test", "b");
        LOG_INFO("Test", "c");
        QCOMPARE(Logging::StructuredLogger::instance().entryCount(), 3);
    }

    void logger_reset()
    {
        LOG_INFO("Test", "before reset");
        QVERIFY(Logging::StructuredLogger::instance().entryCount() > 0);
        Logging::StructuredLogger::instance().reset();
        QCOMPARE(Logging::StructuredLogger::instance().entryCount(), 0);
    }

    // --- Histogram tests ---

    void histogram_empty()
    {
        Metrics::Histogram h;
        QCOMPARE(h.count(), size_t(0));
        QCOMPARE(h.average(), 0.0);
        QCOMPARE(h.percentile(50), 0.0);
    }

    void histogram_singleValue()
    {
        Metrics::Histogram h;
        h.record(42.0);
        QCOMPARE(h.count(), size_t(1));
        QCOMPARE(h.average(), 42.0);
        QCOMPARE(h.percentile(50), 42.0);
        QCOMPARE(h.min(), 42.0);
        QCOMPARE(h.max(), 42.0);
    }

    void histogram_multipleValues()
    {
        Metrics::Histogram h;
        for (int i = 1; i <= 100; ++i) h.record(i);

        QCOMPARE(h.count(), size_t(100));
        QCOMPARE(h.average(), 50.5);
        QCOMPARE(h.min(), 1.0);
        QCOMPARE(h.max(), 100.0);

        double p50 = h.percentile(50);
        QVERIFY(p50 >= 49 && p50 <= 51);

        double p99 = h.percentile(99);
        QVERIFY(p99 >= 98 && p99 <= 100);
    }

    void histogram_reset()
    {
        Metrics::Histogram h;
        h.record(1.0);
        h.record(2.0);
        h.reset();
        QCOMPARE(h.count(), size_t(0));
        QCOMPARE(h.average(), 0.0);
    }

    // --- MetricsCollector tests ---

    void metrics_ordersPlaced()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderPlaced("AAPL");
        mc.recordOrderPlaced("MSFT");
        mc.recordOrderPlaced("AAPL");

        auto snap = mc.snapshot();
        QCOMPARE(snap.ordersPlaced, uint64_t(3));
    }

    void metrics_ordersFilled()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderFilled("AAPL");
        mc.recordOrderFilled("GOOG");

        auto snap = mc.snapshot();
        QCOMPARE(snap.ordersFilled, uint64_t(2));
    }

    void metrics_ordersRejected()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderRejected("TSLA");

        auto snap = mc.snapshot();
        QCOMPARE(snap.ordersRejected, uint64_t(1));
    }

    void metrics_ticksProcessed()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        for (int i = 0; i < 1000; ++i) mc.recordTickProcessed();

        auto snap = mc.snapshot();
        QCOMPARE(snap.ticksProcessed, uint64_t(1000));
    }

    void metrics_signalsGenerated()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordSignalGenerated();
        mc.recordSignalGenerated();

        auto snap = mc.snapshot();
        QCOMPARE(snap.signalsGenerated, uint64_t(2));
    }

    void metrics_riskRejections()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordRiskRejection();

        auto snap = mc.snapshot();
        QCOMPARE(snap.riskRejections, uint64_t(1));
    }

    void metrics_orderLatency()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderLatencyUs(100.0);
        mc.recordOrderLatencyUs(200.0);
        mc.recordOrderLatencyUs(300.0);

        auto snap = mc.snapshot();
        QCOMPARE(snap.avgOrderLatencyUs, 200.0);
    }

    void metrics_pipelineLatency()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordPipelineLatencyUs(50.0);
        mc.recordPipelineLatencyUs(150.0);

        auto snap = mc.snapshot();
        QCOMPARE(snap.avgPipelineLatencyUs, 100.0);
    }

    void metrics_strategyPnL()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordStrategyPnL("momentum-aapl", 1250.50);
        mc.recordStrategyPnL("meanrev-msft", -320.00);

        auto snap = mc.snapshot();
        QCOMPARE(snap.strategyPnL.size(), 2);
        QCOMPARE(snap.strategyPnL["momentum-aapl"], 1250.50);
        QCOMPARE(snap.strategyPnL["meanrev-msft"], -320.00);
    }

    void metrics_reset()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderPlaced("X");
        mc.recordTickProcessed();
        mc.recordStrategyPnL("s1", 100.0);
        mc.reset();

        auto snap = mc.snapshot();
        QCOMPARE(snap.ordersPlaced, uint64_t(0));
        QCOMPARE(snap.ticksProcessed, uint64_t(0));
        QVERIFY(snap.strategyPnL.isEmpty());
    }

    void metrics_snapshot_complete()
    {
        auto& mc = Metrics::MetricsCollector::instance();
        mc.recordOrderPlaced("AAPL");
        mc.recordOrderFilled("AAPL");
        mc.recordOrderRejected("AAPL");
        mc.recordTickProcessed();
        mc.recordSignalGenerated();
        mc.recordRiskRejection();
        mc.recordOrderLatencyUs(500.0);
        mc.recordPipelineLatencyUs(250.0);
        mc.recordStrategyPnL("test", 50.0);

        auto snap = mc.snapshot();
        QCOMPARE(snap.ordersPlaced, uint64_t(1));
        QCOMPARE(snap.ordersFilled, uint64_t(1));
        QCOMPARE(snap.ordersRejected, uint64_t(1));
        QCOMPARE(snap.ticksProcessed, uint64_t(1));
        QCOMPARE(snap.signalsGenerated, uint64_t(1));
        QCOMPARE(snap.riskRejections, uint64_t(1));
        QCOMPARE(snap.avgOrderLatencyUs, 500.0);
        QCOMPARE(snap.avgPipelineLatencyUs, 250.0);
        QCOMPARE(snap.strategyPnL["test"], 50.0);
    }
};

#endif // TST_OBSERVABILITY_H
