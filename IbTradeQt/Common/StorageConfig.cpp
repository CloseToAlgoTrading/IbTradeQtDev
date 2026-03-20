#include "StorageConfig.h"
#include <QFile>
#include <QSettings>

static QString normalizeBackend(const QString& s)
{
    return s.trimmed().toLower();
}

bool StorageConfig::parseBackend(const QString& s, StorageBackend* out, QString* errorOut)
{
    const QString n = normalizeBackend(s);
    if (n == QLatin1String("sqlite") || n.isEmpty()) {
        *out = StorageBackend::Sqlite;
        return true;
    }
    if (n == QLatin1String("postgresql") || n == QLatin1String("postgres")) {
        *out = StorageBackend::Postgresql;
        return true;
    }
    if (errorOut)
        *errorOut = QStringLiteral("backend must be sqlite or postgresql");
    return false;
}

QString StorageConfig::backendToString(StorageBackend b)
{
    switch (b) {
    case StorageBackend::Postgresql:
        return QStringLiteral("postgresql");
    case StorageBackend::Sqlite:
    default:
        return QStringLiteral("sqlite");
    }
}

StorageConfig StorageConfig::loadDefaults()
{
    StorageConfig c;
    c.modelStore.backend = StorageBackend::Sqlite;
    c.modelStore.sqlitePath = QStringLiteral("model_tree.sqlite");
    c.modelStore.postgres.host = QStringLiteral("localhost");
    c.modelStore.postgres.port = 5432;
    c.modelStore.postgres.database = QStringLiteral("IbTrade");
    c.modelStore.postgres.user = QStringLiteral("postgres");
    c.modelStore.postgres.password = QStringLiteral("passw");

    c.appDataStore.path = QStringLiteral("myLocalDb.sqlite");
    c.backtestStore.path = QStringLiteral("myLocalDb.sqlite");

    c.legacyMarketDataDb = c.modelStore.postgres;
    return c;
}

StorageConfig StorageConfig::loadFromIni(const QString& iniPath)
{
    StorageConfig c = loadDefaults();

    if (!QFile::exists(iniPath))
        return c;

    QSettings st(iniPath, QSettings::IniFormat);

    st.beginGroup(QStringLiteral("DBSettings"));
    c.legacyMarketDataDb.host = st.value(QStringLiteral("dbaddress"), c.legacyMarketDataDb.host).toString();
    c.legacyMarketDataDb.port = st.value(QStringLiteral("dbport"), c.legacyMarketDataDb.port).toInt();
    c.legacyMarketDataDb.database = st.value(QStringLiteral("dbname"), c.legacyMarketDataDb.database).toString();
    c.legacyMarketDataDb.user = st.value(QStringLiteral("dbusr"), c.legacyMarketDataDb.user).toString();
    c.legacyMarketDataDb.password = st.value(QStringLiteral("pswd"), c.legacyMarketDataDb.password).toString();
    st.endGroup();

    // Seed ModelStore PG from legacy if section missing
    c.modelStore.postgres = c.legacyMarketDataDb;

    st.beginGroup(QStringLiteral("ModelStore"));
    if (st.contains(QStringLiteral("backend"))) {
        QString err;
        parseBackend(st.value(QStringLiteral("backend")).toString(), &c.modelStore.backend, &err);
    }
    c.modelStore.sqlitePath = st.value(QStringLiteral("sqlite_path"),
                                       st.value(QStringLiteral("path"), c.modelStore.sqlitePath)).toString();
    c.modelStore.postgres.host = st.value(QStringLiteral("pg_host"), c.modelStore.postgres.host).toString();
    c.modelStore.postgres.port = st.value(QStringLiteral("pg_port"), c.modelStore.postgres.port).toInt();
    c.modelStore.postgres.database = st.value(QStringLiteral("pg_database"), c.modelStore.postgres.database).toString();
    c.modelStore.postgres.user = st.value(QStringLiteral("pg_user"), c.modelStore.postgres.user).toString();
    c.modelStore.postgres.password = st.value(QStringLiteral("pg_password"), c.modelStore.postgres.password).toString();
    st.endGroup();

    st.beginGroup(QStringLiteral("AppDataStore"));
    c.appDataStore.path = st.value(QStringLiteral("path"), c.appDataStore.path).toString();
    st.endGroup();

    st.beginGroup(QStringLiteral("BacktestStore"));
    c.backtestStore.path = st.value(QStringLiteral("path"), c.backtestStore.path).toString();
    st.endGroup();

    return c;
}

void StorageConfig::saveToIni(const QString& iniPath, const StorageConfig& cfg)
{
    QSettings st(iniPath, QSettings::IniFormat);

    // Keep legacy [DBSettings] (DBConnector / market) aligned with ModelStore PostgreSQL profile.
    StorageConfig aligned = cfg;
    aligned.legacyMarketDataDb = cfg.modelStore.postgres;

    st.beginGroup(QStringLiteral("ModelStore"));
    st.setValue(QStringLiteral("backend"), backendToString(aligned.modelStore.backend));
    st.setValue(QStringLiteral("sqlite_path"), aligned.modelStore.sqlitePath);
    st.setValue(QStringLiteral("pg_host"), aligned.modelStore.postgres.host);
    st.setValue(QStringLiteral("pg_port"), aligned.modelStore.postgres.port);
    st.setValue(QStringLiteral("pg_database"), aligned.modelStore.postgres.database);
    st.setValue(QStringLiteral("pg_user"), aligned.modelStore.postgres.user);
    st.setValue(QStringLiteral("pg_password"), aligned.modelStore.postgres.password);
    st.endGroup();

    st.beginGroup(QStringLiteral("AppDataStore"));
    st.setValue(QStringLiteral("path"), aligned.appDataStore.path);
    st.endGroup();

    st.beginGroup(QStringLiteral("BacktestStore"));
    st.setValue(QStringLiteral("path"), aligned.backtestStore.path);
    st.endGroup();

    st.beginGroup(QStringLiteral("DBSettings"));
    st.setValue(QStringLiteral("dbaddress"), aligned.legacyMarketDataDb.host);
    st.setValue(QStringLiteral("dbport"), aligned.legacyMarketDataDb.port);
    st.setValue(QStringLiteral("dbname"), aligned.legacyMarketDataDb.database);
    st.setValue(QStringLiteral("dbusr"), aligned.legacyMarketDataDb.user);
    st.setValue(QStringLiteral("pswd"), aligned.legacyMarketDataDb.password);
    st.endGroup();

    st.sync();
}
