#ifndef BACKTESTPRESENTER_H
#define BACKTESTPRESENTER_H

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include "ViewModels.h"
#include "Backtest/BacktestDataTypes.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/LedgerSnapshot.h"
#include "DB/dbdatatypes.h"

class BacktestPresenter : public QObject
{
    Q_OBJECT
public:
    explicit BacktestPresenter(QObject* parent = nullptr);

    // --- Strategy context ---
    void setStrategyContext(const QString& strategyId,
                           const QString& displayName,
                           const QString& portfolioPath,
                           const QString& strategyDefId,
                           int strategyVersion);

    QString headerHtml() const;
    QString currentStrategyId() const { return m_strategyId; }

    // --- Pipeline config ---
    void setPipelineConfig(const QJsonObject& config);
    QJsonObject pipelineConfig() const { return m_pipelineConfig; }
    void mergePipelineConfig(const QJsonObject& updatedConfig);

    // --- Trade log conversion ---
    static QList<VM::TradeRow> convertFills(const QVector<Backtest::FilledOrder>& fills);
    static QList<VM::TradeRow> convertDbTrades(const QList<DbBacktestTrade>& trades);

    // --- Run history conversion ---
    static QList<VM::RunHistoryRow> convertRunHistory(const QList<DbBacktestRunSummary>& runs);

    // --- Result decomposition ---
    struct ChartData {
        QVector<Backtest::LedgerSnapshot> equityCurve;
        QVector<Backtest::LedgerSnapshot> benchmarkCurve;
        QString benchmarkSymbol;
        QMap<QString, QList<DbHistoricalBar>> candleBars;
        QList<DbBacktestTrade> candleFills;
    };

    static ChartData extractChartData(const Backtest::BacktestLoadedRun& run);

signals:
    void headerChanged(const QString& html);
    void pipelineConfigChanged(const QJsonObject& config);

private:
    QString m_strategyId;
    QString m_displayName;
    QString m_portfolioPath;
    QString m_strategyDefId;
    int     m_strategyVersion = 1;
    QJsonObject m_pipelineConfig;
};

#endif // BACKTESTPRESENTER_H
