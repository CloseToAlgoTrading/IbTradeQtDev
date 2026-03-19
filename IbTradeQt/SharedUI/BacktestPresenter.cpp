#include "BacktestPresenter.h"
#include <QDateTime>
#include <cmath>

BacktestPresenter::BacktestPresenter(QObject* parent)
    : QObject(parent)
{
}

void BacktestPresenter::setStrategyContext(const QString& strategyId,
                                           const QString& displayName,
                                           const QString& portfolioPath,
                                           const QString& strategyDefId,
                                           int strategyVersion)
{
    m_strategyId      = strategyId;
    m_displayName     = displayName;
    m_portfolioPath   = portfolioPath;
    m_strategyDefId   = strategyDefId;
    m_strategyVersion = strategyVersion > 0 ? strategyVersion : 1;
    emit headerChanged(headerHtml());
}

QString BacktestPresenter::headerHtml() const
{
    QString versionBadge;
    if (!m_strategyDefId.isEmpty()) {
        versionBadge = QString(
            "<span style='color:#4a90d9; font-size:10px;'>"
            "&nbsp;|&nbsp;def v%1</span>")
            .arg(m_strategyVersion);
    }
    return QString(
        "<b>%1</b>"
        "<span style='color:#777; font-size:11px;'>&nbsp;&nbsp;%2</span>"
        "<span style='color:#aaa; font-size:10px;'>&nbsp;|&nbsp;ID: %3</span>"
        "%4")
        .arg(m_displayName, m_portfolioPath, m_strategyId.left(8), versionBadge);
}

void BacktestPresenter::setPipelineConfig(const QJsonObject& config)
{
    m_pipelineConfig = config;
}

void BacktestPresenter::mergePipelineConfig(const QJsonObject& updatedConfig)
{
    m_pipelineConfig = updatedConfig;
    emit pipelineConfigChanged(m_pipelineConfig);
}

// --- Trade log ---

static const QColor kBuyColor  { 0xd4, 0xed, 0xda };
static const QColor kSellColor { 0xf8, 0xd7, 0xda };

QList<VM::TradeRow> BacktestPresenter::convertFills(const QVector<Backtest::FilledOrder>& fills)
{
    QList<VM::TradeRow> rows;
    rows.reserve(fills.size());
    for (const auto& f : fills) {
        VM::TradeRow row;
        row.timestamp = f.timestamp;
        row.symbol    = f.symbol;
        row.quantity  = std::abs(f.quantity);
        row.price     = f.fillPrice;
        row.side      = f.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
        row.sideColor = f.quantity > 0 ? kBuyColor : kSellColor;
        rows.append(row);
    }
    return rows;
}

QList<VM::TradeRow> BacktestPresenter::convertDbTrades(const QList<DbBacktestTrade>& trades)
{
    QList<VM::TradeRow> rows;
    rows.reserve(trades.size());
    for (const auto& t : trades) {
        VM::TradeRow row;
        row.timestamp = QDateTime::fromString(t.timestamp, Qt::ISODate);
        row.symbol    = t.symbol;
        row.side      = t.side;
        row.quantity  = t.quantity;
        row.price     = t.fillPrice;
        row.sideColor = (t.side == QLatin1String("BUY")) ? kBuyColor : kSellColor;
        rows.append(row);
    }
    return rows;
}

// --- Run history ---

QList<VM::RunHistoryRow> BacktestPresenter::convertRunHistory(const QList<DbBacktestRunSummary>& runs)
{
    QList<VM::RunHistoryRow> rows;
    rows.reserve(runs.size());
    for (const auto& r : runs) {
        VM::RunHistoryRow row;
        row.runId       = r.runId;
        row.status      = r.status;
        row.startedAt   = r.startDate;
        row.finishedAt  = r.endDate;
        row.totalReturn = r.totalReturn;
        row.sharpe      = r.sharpeRatio;

        if (r.status == Backtest::Status::Finished)
            row.statusColor = QColor(0xd4, 0xed, 0xda);
        else if (r.status == Backtest::Status::Failed)
            row.statusColor = QColor(0xf8, 0xd7, 0xda);
        else if (r.status == Backtest::Status::Running)
            row.statusColor = QColor(0xff, 0xf3, 0xcd);

        rows.append(row);
    }
    return rows;
}

// --- Chart data extraction ---

BacktestPresenter::ChartData BacktestPresenter::extractChartData(const Backtest::BacktestLoadedRun& run)
{
    ChartData data;
    data.equityCurve    = run.result.equityCurve;
    data.benchmarkCurve = run.result.benchmark.equityCurve;
    data.benchmarkSymbol = run.result.benchmark.symbol;

    for (const auto& fill : run.result.tradeLog) {
        DbBacktestTrade t;
        t.runId     = run.record.runId;
        t.symbol    = fill.symbol;
        t.side      = fill.quantity > 0 ? QStringLiteral("BUY") : QStringLiteral("SELL");
        t.quantity  = std::abs(fill.quantity);
        t.fillPrice = fill.fillPrice;
        t.timestamp = fill.timestamp.toUTC().toString(Qt::ISODate);
        data.candleFills.append(t);
    }

    data.candleBars = run.histBars;
    return data;
}
