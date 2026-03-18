#ifndef DBHANDLER_H
#define DBHANDLER_H

#include "dbdatatypes.h"
#include <QObject>
#include <QtSql/QSqlDatabase>

class DBHandler : public QObject {
    Q_OBJECT

public:
    explicit DBHandler(QObject *parent = nullptr);
    ~DBHandler();

    bool connectDB(const QString& dbName);
    void disconnectDB();

    bool createTrigger(QSqlDatabase& db);

private:
    QString m_uniqueConnectionName;

signals:
    void signalOpenPositionsFetched(const QList<OpenPosition>& positions, e_queryStatus state);
    void signalModelInfoFetched(const DbModelInfo& obj, e_queryStatus state);
    void signalStrategyDataFetched(const DbStrategyData& obj, e_queryStatus state);
    void signalDBConnectionState(const bool state);

    // Backtest signals
    void signalRunsForStrategyFetched(const QList<DbBacktestRunSummary>& runs);
    void signalLoadedRunFetched(const DbBacktestRun& run,
                                const DbBacktestMetrics& metrics,
                                const QList<DbBacktestTrade>& trades,
                                const QList<DbBacktestEquityPoint>& equity);
    void signalHistoricalBarsFetched(const QString& symbol,
                                     const QString& resolution,
                                     const QString& dataSourceId,
                                     const QList<DbHistoricalBar>& bars);
    void signalCachedBarRangeFetched(const QString& symbol,
                                     const QString& resolution,
                                     const QString& dataSourceId,
                                     const QString& minTs,
                                     const QString& maxTs);

    // Backtest run profile signals
    void signalRunProfilesFetched(const QList<DbBacktestRunProfile>& profiles);
    void signalRunsForDefinitionFetched(const QList<DbBacktestRunSummary>& runs);

public slots:
    // Live trading slots (existing)
    void slotAddPositionQuery(const OpenPosition& position);
    void slotAddNewTrade(const DbTrade& trade);
    void slotUpdateTradeCommission(const DbTradeCommission& tradeComm);
    void initializeConnectionSlot();
    void slotFetchOpenPositions(const QString& strategy_id);
    void slotAddOrUpdateDbModelInfo(const DbModelInfo& obj);
    void slotGetModelInfo(const QString& modelId);
    void slotAddOrUpdateDbStrategyData(const DbStrategyData& obj);
    void slotGetStrategyData(const QString& strategy_id);

    // Backtest write slots
    void slotInsertBacktestRun(const DbBacktestRun& run);
    void slotUpdateBacktestRunStatus(const QString& runId, const QString& status,
                                     const QString& errorText, qint64 durationMs,
                                     const QString& dataRefreshedAt);
    void slotInsertBacktestMetrics(const DbBacktestMetrics& metrics);
    void slotInsertBacktestTrades(const QList<DbBacktestTrade>& trades);
    void slotInsertEquityCurve(const QList<DbBacktestEquityPoint>& points);
    void slotUpsertHistoricalBars(const QList<DbHistoricalBar>& bars);

    // Backtest read slots (async — emit signals with results)
    void slotFetchRunsForStrategy(const QString& strategyId);
    void slotFetchLoadedRun(const QString& runId);
    void slotFetchHistoricalBars(const QString& symbol, const QString& resolution,
                                  const QString& dataSourceId,
                                  const QString& fromUtc, const QString& toUtc);
    void slotFetchCachedBarRange(const QString& symbol, const QString& resolution,
                                  const QString& dataSourceId);

    // Backtest run profile slots
    void slotInsertBacktestRunProfile(const DbBacktestRunProfile& profile);
    void slotFetchRunProfilesForOwner(const QString& ownerType, const QString& ownerRefId);
    void slotFetchRunsForDefinition(const QString& strategyDefId);

private:
    QSqlDatabase m_db;
    bool initializeDatabase();
    void initializeBacktestTables();
};

#endif // DBHANDLER_H
