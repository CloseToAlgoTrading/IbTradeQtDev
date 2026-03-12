#ifndef TST_REPLAY_H
#define TST_REPLAY_H

#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QDir>
#include "Replay/MarketDataRecorder.h"
#include "Replay/MarketDataReplayer.h"

class TestReplay : public QObject
{
    Q_OBJECT

private slots:
    void recorderWritesTicks()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            QVERIFY(recorder.isOpen());

            IBComm::MarketTick t;
            t.symbol = "AAPL";
            t.bid = 149.0;
            t.ask = 150.0;
            t.timestamp = QDateTime(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());

            recorder.onMarketTick(t);
            recorder.onMarketTick(t);
            QCOMPARE(recorder.tickCount(), 2);
        }

        QFile f(filename);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArrayList lines;
        while (!f.atEnd()) {
            QByteArray line = f.readLine().trimmed();
            if (!line.isEmpty()) lines.append(line);
        }
        QCOMPARE(lines.size(), 2);

        QJsonObject obj = QJsonDocument::fromJson(lines[0]).object();
        QCOMPARE(obj["type"].toString(), QString("MarketTick"));
        QCOMPARE(obj["symbol"].toString(), QString("AAPL"));
        QCOMPARE(obj["bid"].toDouble(), 149.0);
        QCOMPARE(obj["ask"].toDouble(), 150.0);
    }

    void recorderWritesExecutionIntents()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            Pipeline::ExecutionIntent ei;
            ei.symbol = "MSFT";
            ei.quantity = -50.0;
            ei.orderType = Pipeline::ExecutionIntent::Market;
            ei.correlationId = "corr-1";
            ei.timestamp = QDateTime(QDate(2026, 3, 4), QTime(11, 0, 0), QTimeZone::utc());

            recorder.onExecutionIntent(ei);
            QCOMPARE(recorder.intentCount(), 1);
        }

        QFile f(filename);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonObject obj = QJsonDocument::fromJson(f.readLine()).object();
        QCOMPARE(obj["type"].toString(), QString("ExecutionIntent"));
        QCOMPARE(obj["quantity"].toDouble(), -50.0);
        QCOMPARE(obj["correlationId"].toString(), QString("corr-1"));
    }

    void recorderWritesBlockGraphConfig()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            QJsonObject config;
            config["alphas"] = "momentum";
            recorder.writeBlockGraphConfig(config);
        }

        QFile f(filename);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonObject obj = QJsonDocument::fromJson(f.readLine()).object();
        QCOMPARE(obj["type"].toString(), QString("BlockGraphConfig"));
        QCOMPARE(obj["config"].toObject()["alphas"].toString(), QString("momentum"));
    }

    void replayerLoadsGoldenFile()
    {
        QString testFile = QDir(QCoreApplication::applicationDirPath())
            .filePath("../../data/golden_test_001.jsonl");
        if (!QFile::exists(testFile)) {
            testFile = QString(SRCDIR) + "/data/golden_test_001.jsonl";
        }
        if (!QFile::exists(testFile)) {
            QSKIP("Golden test file not found");
        }

        MarketDataReplayer replayer(testFile);
        QCOMPARE(replayer.tickCount(), 8);
        QVERIFY(!replayer.blockGraphConfig().isEmpty());
    }

    void replayerEmitsTickSignals()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            IBComm::MarketTick t1;
            t1.symbol = "AAPL"; t1.bid = 149.0; t1.ask = 150.0;
            t1.timestamp = QDateTime(QDate(2026, 3, 4), QTime(10, 0, 0), QTimeZone::utc());

            IBComm::MarketTick t2;
            t2.symbol = "MSFT"; t2.bid = 300.0; t2.ask = 301.0;
            t2.timestamp = QDateTime(QDate(2026, 3, 4), QTime(10, 1, 0), QTimeZone::utc());

            recorder.onMarketTick(t1);
            recorder.onMarketTick(t2);
        }

        MarketDataReplayer replayer(filename);
        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&replayer, &MarketDataReplayer::tick);

        replayer.replay();

        QCOMPARE(spy.count(), 2);
        auto tick1 = spy.at(0).at(0).value<IBComm::MarketTick>();
        auto tick2 = spy.at(1).at(0).value<IBComm::MarketTick>();
        QCOMPARE(tick1.symbol, QString("AAPL"));
        QCOMPARE(tick2.symbol, QString("MSFT"));
    }

    void roundTripRecordReplay()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        QVector<IBComm::MarketTick> original;
        for (int i = 0; i < 5; ++i) {
            IBComm::MarketTick t;
            t.symbol = "TEST";
            t.bid = 100.0 + i;
            t.ask = 100.5 + i;
            t.timestamp = QDateTime(QDate(2026, 3, 4),
                QTime(10, 0, 0), QTimeZone::utc()).addSecs(i * 60);
            original.append(t);
        }

        {
            MarketDataRecorder recorder(filename);
            for (const auto& t : original)
                recorder.onMarketTick(t);
        }

        MarketDataReplayer replayer(filename);
        QCOMPARE(replayer.tickCount(), 5);

        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&replayer, &MarketDataReplayer::tick);
        replayer.replay();

        QCOMPARE(spy.count(), 5);
        for (int i = 0; i < 5; ++i) {
            auto t = spy.at(i).at(0).value<IBComm::MarketTick>();
            QCOMPARE(t.symbol, original[i].symbol);
            QCOMPARE(t.bid, original[i].bid);
            QCOMPARE(t.ask, original[i].ask);
            QCOMPARE(t.timestamp, original[i].timestamp);
        }
    }

    void replayerHandlesEmptyFile()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        QFile f(filename);
        f.open(QIODevice::WriteOnly);
        f.close();

        MarketDataReplayer replayer(filename);
        QCOMPARE(replayer.tickCount(), 0);
    }

    void replayerSkipsMalformedLines()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        QFile f(filename);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        f.write("{\"type\":\"MarketTick\",\"symbol\":\"AAPL\",\"bid\":149.0,\"ask\":150.0,\"timestamp\":\"2026-03-04T10:00:00.000Z\"}\n");
        f.write("this is not json\n");
        f.write("{\"type\":\"MarketTick\",\"symbol\":\"MSFT\",\"bid\":300.0,\"ask\":301.0,\"timestamp\":\"2026-03-04T10:01:00.000Z\"}\n");
        f.close();

        MarketDataReplayer replayer(filename);
        QCOMPARE(replayer.tickCount(), 2);
    }

    void replayUntilTimestamp()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            for (int i = 0; i < 5; ++i) {
                IBComm::MarketTick t;
                t.symbol = "AAPL"; t.bid = 100.0 + i; t.ask = 100.5 + i;
                t.timestamp = QDateTime(QDate(2026, 3, 4),
                    QTime(10, 0, 0), QTimeZone::utc()).addSecs(i * 60);
                recorder.onMarketTick(t);
            }
        }

        MarketDataReplayer replayer(filename);
        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");
        QSignalSpy spy(&replayer, &MarketDataReplayer::tick);

        QDateTime cutoff = QDateTime(QDate(2026, 3, 4),
            QTime(10, 2, 0), QTimeZone::utc());
        replayer.replayUntil(cutoff);

        QCOMPARE(spy.count(), 3);
    }

    void replayDeterministic()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString filename = tmpFile.fileName();
        tmpFile.close();

        {
            MarketDataRecorder recorder(filename);
            for (int i = 0; i < 3; ++i) {
                IBComm::MarketTick t;
                t.symbol = "AAPL"; t.bid = 100.0 + i; t.ask = 100.5 + i;
                t.timestamp = QDateTime(QDate(2026, 3, 4),
                    QTime(10, 0, 0), QTimeZone::utc()).addSecs(i * 60);
                recorder.onMarketTick(t);
            }
        }

        qRegisterMetaType<IBComm::MarketTick>("IBComm::MarketTick");

        // Replay twice, results must be identical
        QVector<IBComm::MarketTick> run1, run2;
        {
            MarketDataReplayer replayer(filename);
            QSignalSpy spy(&replayer, &MarketDataReplayer::tick);
            replayer.replay();
            for (int i = 0; i < spy.count(); ++i)
                run1.append(spy.at(i).at(0).value<IBComm::MarketTick>());
        }
        {
            MarketDataReplayer replayer(filename);
            QSignalSpy spy(&replayer, &MarketDataReplayer::tick);
            replayer.replay();
            for (int i = 0; i < spy.count(); ++i)
                run2.append(spy.at(i).at(0).value<IBComm::MarketTick>());
        }

        QCOMPARE(run1.size(), run2.size());
        for (int i = 0; i < run1.size(); ++i) {
            QCOMPARE(run1[i].symbol, run2[i].symbol);
            QCOMPARE(run1[i].bid, run2[i].bid);
            QCOMPARE(run1[i].ask, run2[i].ask);
            QCOMPARE(run1[i].timestamp, run2[i].timestamp);
        }
    }
};

#endif // TST_REPLAY_H
