#ifndef TST_STORAGE_CONFIG_H
#define TST_STORAGE_CONFIG_H

#include <QObject>
#include <QtTest>
#include <QTemporaryFile>
#include <QTextStream>
#include "StorageConfig.h"

class TestStorageConfig : public QObject
{
    Q_OBJECT
private slots:
    void defaults()
    {
        StorageConfig c = StorageConfig::loadDefaults();
        QCOMPARE(c.modelStore.backend, StorageBackend::Sqlite);
        QCOMPARE(c.modelStore.sqlitePath, QStringLiteral("model_tree.sqlite"));
        QCOMPARE(c.appDataStore.path, QStringLiteral("myLocalDb.sqlite"));
        QCOMPARE(c.backtestStore.path, QStringLiteral("myLocalDb.sqlite"));
    }

    void loadLegacyDbSettingsMergesIntoModelStore()
    {
        QTemporaryFile f;
        QVERIFY(f.open());
        QTextStream out(&f);
        out << "[DBSettings]\n";
        out << "dbaddress=legacy.host\n";
        out << "dbport=5555\n";
        out << "dbname=LegacyDb\n";
        out << "dbusr=legacyuser\n";
        out << "pswd=legacysecret\n";
        f.close();

        StorageConfig c = StorageConfig::loadFromIni(f.fileName());
        QCOMPARE(c.legacyMarketDataDb.host, QStringLiteral("legacy.host"));
        QCOMPARE(c.legacyMarketDataDb.port, 5555);
        QCOMPARE(c.modelStore.postgres.host, QStringLiteral("legacy.host"));
        QCOMPARE(c.modelStore.postgres.database, QStringLiteral("LegacyDb"));
    }

    void saveRoundTrip()
    {
        QTemporaryFile f;
        QVERIFY(f.open());
        f.close();

        StorageConfig c = StorageConfig::loadDefaults();
        c.modelStore.sqlitePath = QStringLiteral("custom_model.sqlite");
        c.modelStore.postgres.host = QStringLiteral("pg.example");
        c.modelStore.postgres.port = 5433;
        c.appDataStore.path = QStringLiteral("app.sqlite");
        c.backtestStore.path = QStringLiteral("bt.sqlite");

        StorageConfig::saveToIni(f.fileName(), c);
        StorageConfig loaded = StorageConfig::loadFromIni(f.fileName());
        QCOMPARE(loaded.modelStore.sqlitePath, QStringLiteral("custom_model.sqlite"));
        QCOMPARE(loaded.appDataStore.path, QStringLiteral("app.sqlite"));
        QCOMPARE(loaded.backtestStore.path, QStringLiteral("bt.sqlite"));
        QCOMPARE(loaded.modelStore.postgres.host, QStringLiteral("pg.example"));
        QCOMPARE(loaded.legacyMarketDataDb.host, loaded.modelStore.postgres.host);
        QCOMPARE(loaded.legacyMarketDataDb.port, loaded.modelStore.postgres.port);
    }

    void parseBackend_strings()
    {
        StorageBackend b = StorageBackend::Sqlite;
        QString err;
        QVERIFY(StorageConfig::parseBackend(QStringLiteral("sqlite"), &b, &err));
        QCOMPARE(b, StorageBackend::Sqlite);
        QVERIFY(StorageConfig::parseBackend(QStringLiteral("POSTGRESQL"), &b, &err));
        QCOMPARE(b, StorageBackend::Postgresql);
        QVERIFY(!StorageConfig::parseBackend(QStringLiteral("oracle"), &b, &err));
    }
};

#endif
