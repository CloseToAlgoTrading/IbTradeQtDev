#ifndef BACKTEST_REPORTING_MOMENTUMSEMANTICHTMLWRITER_H
#define BACKTEST_REPORTING_MOMENTUMSEMANTICHTMLWRITER_H

#include "Backtest/BacktestResult.h"
#include "Backtest/FilledOrder.h"
#include "Backtest/HistoricalBarUtils.h"
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/DataQuality.h"
#include "IBComm/HistoricalDataRouter.h"

#include <QDateTime>
#include <QMap>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

namespace MomentumSemanticValidationHtml {

struct Params {
    QString documentTitle;
    QString pageHeading;
    QString primaryInfoboxInnerHtml;
    QString introParagraph1;
    QString introParagraph2;
    QString introParagraph3;
    int     topN                    = 5;
    int     tradingDaysBlurb        = 0;
    QStringList universe;
    bool    fullOhlcGridPerTrade    = true;
    bool    includeCorrelationColumn = false;
};

QString htmlEsc(const QString& s);

inline const IBComm::HistoricalBar* barForSessionDay(const QVector<IBComm::HistoricalBar>& series,
                                                     const QDateTime&                    tradeUtc)
{
    return Backtest::barForSessionDay(series, tradeUtc);
}

QString svgCloseChart(const QString& symbol,
                      const QVector<IBComm::HistoricalBar>& barsInWindow,
                      const QVector<Backtest::FilledOrder>&   trades);

QVector<Backtest::LedgerSnapshot> sortedSnapshotsForChart(const QVector<Backtest::LedgerSnapshot>& v);
QVector<Backtest::LedgerSnapshot> lastSnapshotPerUtcDay(const QVector<Backtest::LedgerSnapshot>& sortedChrono);
QString                           svgPathCubicSmooth(const QVector<QPointF>& pts);
void writeSvgStrategyVsBenchmark(QTextStream& html, const QString& title,
                                 const QVector<Backtest::LedgerSnapshot>& strategy,
                                 const QVector<Backtest::LedgerSnapshot>& benchmark,
                                 const QString& benchLabel, double initialCapital, bool pnlMode);
QString dataQualityLabel(Backtest::DataQuality q);
QDateTime firstLongBuyTime(const QVector<Backtest::FilledOrder>& trades);
QVector<IBComm::HistoricalBar> barsForSchedule(const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars,
                                               const QStringList& syms);
QString holdingsNonZeroListHtml(const QStringList& syms, const QMap<QString, double>& pos);

void writeReport(const Backtest::BacktestResult& result,
                 const QString& path,
                 const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars,
                 const QDateTime& windowStart,
                 const QDateTime& windowEnd,
                 int     rebalanceEveryNBars,
                 double  initialCapitalCfg,
                 double  maxPositionSharesRisk,
                 const Params& p);

} // namespace MomentumSemanticValidationHtml

#endif
