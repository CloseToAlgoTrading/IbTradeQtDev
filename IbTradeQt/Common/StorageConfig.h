#ifndef STORAGECONFIG_H
#define STORAGECONFIG_H

#include <QString>

/** Logical backend for a store (see docs/ARCHITECTURE.md — storage topology). */
enum class StorageBackend {
    Sqlite,
    Postgresql,
};

/** PostgreSQL connection parameters (shared by ModelStore when backend is Postgresql, and legacy market-data code). */
struct PostgreSqlConnection {
    QString host;
    int port = 5432;
    QString database;
    QString user;
    QString password;
};

struct ModelStoreConfig {
    StorageBackend backend = StorageBackend::Sqlite;
    QString sqlitePath; // used when backend == Sqlite
    PostgreSqlConnection postgres;
};

struct FileBackedStoreConfig {
    QString path;
};

/** Loaded once at startup from ibtrade.ini; do not parse INI inside individual repositories. */
struct StorageConfig {
    ModelStoreConfig modelStore;
    FileBackedStoreConfig appDataStore;
    FileBackedStoreConfig backtestStore;
    /** Legacy [DBSettings] — market / DBConnector; kept distinct from ModelStore. */
    PostgreSqlConnection legacyMarketDataDb;

    static StorageConfig loadDefaults();
    /** Reads ibtrade.ini; merges legacy [DBSettings] when new sections are absent. */
    static StorageConfig loadFromIni(const QString& iniPath);
    /** Persists user-editable storage keys (used by Settings UI). */
    static void saveToIni(const QString& iniPath, const StorageConfig& cfg);

    static bool parseBackend(const QString& s, StorageBackend* out, QString* errorOut);
    static QString backendToString(StorageBackend b);
};

#endif
