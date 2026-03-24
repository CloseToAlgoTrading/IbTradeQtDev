#include "Backtest/Reporting/MomentumSemanticHtmlWriter.h"

#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QDateTime>
#include <QPointF>
#include <QMap>
#include <QVector>
#include <QStringList>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <limits>

namespace MomentumSemanticValidationHtml {

QString htmlEsc(const QString& s)
{
    QString r;
    r.reserve(s.size() + 8);
    for (QChar c : s) {
        switch (c.unicode()) {
        case '&':  r += QStringLiteral("&amp;");  break;
        case '<':  r += QStringLiteral("&lt;");   break;
        case '>':  r += QStringLiteral("&gt;");   break;
        case '"':  r += QStringLiteral("&quot;"); break;
        default: r += c; break;
        }
    }
    return r;
}

QString svgCloseChart(const QString& symbol,
                             const QVector<IBComm::HistoricalBar>& barsInWindow,
                             const QVector<Backtest::FilledOrder>& trades)
{
    const int W    = 920;
    const int H    = 260;
    const int padL = 48;
    const int padR = 16;
    const int padT = 28;
    const int padB = 36;

    if (barsInWindow.isEmpty())
        return QStringLiteral("<p>No bars for ") + htmlEsc(symbol) + QStringLiteral("</p>");

    double minP = barsInWindow.first().close;
    double maxP = minP;
    for (const auto& b : barsInWindow) {
        minP = std::min(minP, b.close);
        maxP = std::max(maxP, b.close);
    }
    if (qFuzzyCompare(maxP, minP))
        maxP = minP + 1.0;

    auto xForIndex = [&](int i) {
        const int n = barsInWindow.size();
        if (n <= 1)
            return double(padL);
        return padL + double(i) / double(n - 1) * double(W - padL - padR);
    };

    auto yForPrice = [&](double pr) {
        return padT + (1.0 - (pr - minP) / (maxP - minP)) * double(H - padT - padB);
    };

    QString pathD = QStringLiteral("M ");
    for (int i = 0; i < barsInWindow.size(); ++i) {
        const double x = xForIndex(i);
        const double y = yForPrice(barsInWindow[i].close);
        pathD += QString::number(x, 'f', 2) + QLatin1Char(' ') + QString::number(y, 'f', 2);
        if (i < barsInWindow.size() - 1)
            pathD += QStringLiteral(" L ");
    }

    QString markers;
    for (const auto& t : trades) {
        if (t.symbol.trimmed().toUpper() != symbol)
            continue;
        const IBComm::HistoricalBar* pb = Backtest::barForSessionDay(barsInWindow, t.timestamp);
        if (!pb)
            continue;
        int idx = -1;
        const QDate tradeDay = t.timestamp.toUTC().date();
        for (int i = 0; i < barsInWindow.size(); ++i) {
            if (barsInWindow[i].timestamp.toUTC().date() == tradeDay) {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            continue;
        const double cx = xForIndex(idx);
        const double cy = yForPrice(pb->close);
        const bool   buy = t.quantity > 0.0;
        const QString color = buy ? QStringLiteral("#0a7") : QStringLiteral("#c22");
        markers += QStringLiteral("<circle cx=\"%1\" cy=\"%2\" r=\"5\" fill=\"%3\" stroke=\"#fff\" "
                                  "stroke-width=\"1\"><title>%4 %5 @ hist close %6</title></circle>\n")
                        .arg(cx, 0, 'f', 2)
                        .arg(cy, 0, 'f', 2)
                        .arg(color)
                        .arg(buy ? QStringLiteral("Buy") : QStringLiteral("Sell"))
                        .arg(t.quantity, 0, 'f', 2)
                        .arg(pb->close, 0, 'f', 4);
    }

    QString pathDForArg = htmlEsc(pathD);
    pathDForArg.replace(QLatin1Char('%'), QLatin1String("%%"));
    QString markersForArg = markers;
    markersForArg.replace(QLatin1Char('%'), QLatin1String("%%"));

    return QStringLiteral(
               "<svg width=\"%1\" height=\"%2\" viewBox=\"0 0 %1 %2\" style=\"background:#fff;border:1px solid #ddd;\">"
               "<text x=\"%3\" y=\"20\" font-size=\"14\" font-weight=\"600\">%4 — close (line) / rebalance markers</text>"
               "<text x=\"%3\" y=\"%5\" font-size=\"11\" fill=\"#555\">min %6  max %7</text>"
               "<path d=\"%8\" fill=\"none\" stroke=\"#1a73e8\" stroke-width=\"1.5\"/>"
               "%9"
               "</svg>")
        .arg(W)
        .arg(H)
        .arg(padL)
        .arg(htmlEsc(symbol))
        .arg(H - 8)
        .arg(minP, 0, 'f', 2)
        .arg(maxP, 0, 'f', 2)
        .arg(pathDForArg)
        .arg(markersForArg);
}

QVector<Backtest::LedgerSnapshot> sortedSnapshotsForChart(const QVector<Backtest::LedgerSnapshot>& v)
{
    if (v.size() <= 1)
        return v;
    QVector<Backtest::LedgerSnapshot> sorted = v;
    std::sort(sorted.begin(), sorted.end(),
              [](const Backtest::LedgerSnapshot& a, const Backtest::LedgerSnapshot& b) {
                  return a.timestamp < b.timestamp;
              });
    QVector<Backtest::LedgerSnapshot> out;
    out.reserve(sorted.size());
    for (const auto& s : sorted) {
        if (!out.isEmpty() && out.last().timestamp == s.timestamp)
            out.last() = s;
        else
            out.append(s);
    }
    return out;
}

QVector<Backtest::LedgerSnapshot> lastSnapshotPerUtcDay(
    const QVector<Backtest::LedgerSnapshot>& sortedChrono)
{
    if (sortedChrono.isEmpty())
        return {};
    QVector<Backtest::LedgerSnapshot> out;
    out.reserve(sortedChrono.size());
    for (const auto& s : sortedChrono) {
        const QDate day = s.timestamp.toUTC().date();
        if (out.isEmpty() || out.last().timestamp.toUTC().date() != day)
            out.append(s);
        else
            out.last() = s;
    }
    return out;
}

QString svgPathCubicSmooth(const QVector<QPointF>& pts)
{
    const int n = pts.size();
    if (n == 0)
        return {};
    if (n == 1) {
        return QStringLiteral("M %1 %2").arg(pts[0].x(), 0, 'f', 2).arg(pts[0].y(), 0, 'f', 2);
    }
    if (n == 2) {
        return QStringLiteral("M %1 %2 L %3 %4")
            .arg(pts[0].x(), 0, 'f', 2)
            .arg(pts[0].y(), 0, 'f', 2)
            .arg(pts[1].x(), 0, 'f', 2)
            .arg(pts[1].y(), 0, 'f', 2);
    }
    QString d = QStringLiteral("M %1 %2")
                    .arg(pts[0].x(), 0, 'f', 2)
                    .arg(pts[0].y(), 0, 'f', 2);
    for (int i = 0; i < n - 1; ++i) {
        const double p0x = (i > 0) ? pts[i - 1].x() : pts[i].x();
        const double p0y = (i > 0) ? pts[i - 1].y() : pts[i].y();
        const double p1x = pts[i].x();
        const double p1y = pts[i].y();
        const double p2x = pts[i + 1].x();
        const double p2y = pts[i + 1].y();
        const double p3x = (i + 2 < n) ? pts[i + 2].x() : pts[i + 1].x();
        const double p3y = (i + 2 < n) ? pts[i + 2].y() : pts[i + 1].y();
        const double cp1x = p1x + (p2x - p0x) / 6.0;
        const double cp1y = p1y + (p2y - p0y) / 6.0;
        const double cp2x = p2x - (p3x - p1x) / 6.0;
        const double cp2y = p2y - (p3y - p1y) / 6.0;
        d += QStringLiteral(" C %1 %2 %3 %4 %5 %6")
                 .arg(cp1x, 0, 'f', 2)
                 .arg(cp1y, 0, 'f', 2)
                 .arg(cp2x, 0, 'f', 2)
                 .arg(cp2y, 0, 'f', 2)
                 .arg(p2x, 0, 'f', 2)
                 .arg(p2y, 0, 'f', 2);
    }
    return d;
}

void writeSvgStrategyVsBenchmark(QTextStream& html, const QString& title,
                                      const QVector<Backtest::LedgerSnapshot>& strategy,
                                      const QVector<Backtest::LedgerSnapshot>& benchmark,
                                      const QString& benchLabel, double initialCapital, bool pnlMode)
{
    auto yOf = [&](const Backtest::LedgerSnapshot& s) {
        return pnlMode ? (s.portfolioValue - initialCapital) : s.portfolioValue;
    };
    if (strategy.isEmpty() && benchmark.isEmpty())
        return;

    const QVector<Backtest::LedgerSnapshot> strategyChrono =
        lastSnapshotPerUtcDay(sortedSnapshotsForChart(strategy));
    const QVector<Backtest::LedgerSnapshot> benchmarkChrono = sortedSnapshotsForChart(benchmark);

    qint64 minTs = std::numeric_limits<qint64>::max();
    qint64 maxTs = std::numeric_limits<qint64>::min();
    for (const auto& s : strategyChrono) {
        const qint64 t = s.timestamp.toMSecsSinceEpoch();
        minTs          = std::min(minTs, t);
        maxTs          = std::max(maxTs, t);
    }
    for (const auto& s : benchmarkChrono) {
        const qint64 t = s.timestamp.toMSecsSinceEpoch();
        minTs          = std::min(minTs, t);
        maxTs          = std::max(maxTs, t);
    }
    if (minTs > maxTs)
        return;
    if (minTs == maxTs)
        maxTs = minTs + 1;

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& s : strategyChrono) {
        const double y = yOf(s);
        minY           = std::min(minY, y);
        maxY           = std::max(maxY, y);
    }
    for (const auto& s : benchmarkChrono) {
        const double y = yOf(s);
        minY           = std::min(minY, y);
        maxY           = std::max(maxY, y);
    }
    if (qFuzzyCompare(maxY, minY))
        maxY = minY + 1.0;

    const int W      = 900;
    const int H      = 300;
    const int padLR  = 24;
    const int padTop = 52;
    const int padBot = 20;
    const int innerW = W - 2 * padLR;
    const int innerH = H - padTop - padBot;

    auto pathFor = [&](const QVector<Backtest::LedgerSnapshot>& v) -> QString {
        if (v.isEmpty())
            return {};
        QVector<QPointF> pts;
        pts.reserve(v.size());
        for (int i = 0; i < v.size(); ++i) {
            const qint64 t = v[i].timestamp.toMSecsSinceEpoch();
            const double x = padLR + double(t - minTs) / double(maxTs - minTs) * innerW;
            const double y = padTop + (1.0 - (yOf(v[i]) - minY) / (maxY - minY)) * innerH;
            pts.append(QPointF(x, y));
        }
        return svgPathCubicSmooth(pts);
    };

    html << "<h2>" << htmlEsc(title) << "</h2>\n";
    html << "<svg width=\"" << W << "\" height=\"" << H << "\" viewBox=\"0 0 " << W << " " << H << "\">\n";
    html << "<rect width=\"" << W << "\" height=\"" << H << "\" fill=\"#fafafa\"/>\n";
    html << "<text x=\"" << padLR << "\" y=\"22\" font-size=\"14\" font-weight=\"600\">" << htmlEsc(title)
         << "</text>\n";
    html << "<text x=\"" << padLR << "\" y=\"40\" font-size=\"11\" fill=\"#1a73e8\">— Strategy (cumulative P&amp;L)</text>\n";
    if (!benchmark.isEmpty()) {
        html << "<text x=\"" << (padLR + 220) << "\" y=\"40\" font-size=\"11\" fill=\"#e8710a\">— "
             << htmlEsc(benchLabel) << " (buy-and-hold)</text>\n";
    }
    html << "<text x=\"" << padLR << "\" y=\"" << (H - 8) << "\" font-size=\"10\" fill=\"#777\">Strategy: "
            "end-of-day equity (one point per UTC day); curves are cubic-smoothed for display.</text>\n";

    const QString pathS = pathFor(strategyChrono);
    const QString pathB = pathFor(benchmarkChrono);
    if (!pathS.isEmpty())
        html << "<path d=\"" << pathS << "\" fill=\"none\" stroke=\"#1a73e8\" stroke-width=\"2\" "
                "stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n";
    if (!pathB.isEmpty())
        html << "<path d=\"" << pathB << "\" fill=\"none\" stroke=\"#e8710a\" stroke-width=\"2\" "
                "stroke-dasharray=\"6 4\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n";
    html << "</svg>\n";
}

QString dataQualityLabel(Backtest::DataQuality q)
{
    using DQ = Backtest::DataQuality;
    switch (q) {
    case DQ::RealTicks:       return QStringLiteral("RealTicks");
    case DQ::SynthesizedOHLC: return QStringLiteral("SynthesizedOHLC");
    case DQ::DailyBars:       return QStringLiteral("DailyBars");
    }
    return QStringLiteral("?");
}

QDateTime firstLongBuyTime(const QVector<Backtest::FilledOrder>& trades)
{
    for (const auto& t : trades) {
        if (t.quantity > 0.0)
            return t.timestamp;
    }
    return {};
}

QVector<IBComm::HistoricalBar> barsForSchedule(
    const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars, const QStringList& syms)
{
    for (const QString& s : syms) {
        const QVector<IBComm::HistoricalBar> b = strategyBars.value(s);
        if (!b.isEmpty())
            return b;
    }
    return {};
}

/// Comma-separated "SYM: qty" for symbols with |net| > epsilon; universe order preserved.
QString holdingsNonZeroListHtml(const QStringList& syms, const QMap<QString, double>& pos)
{
    constexpr double eps = 1e-9;
    QStringList      parts;
    for (const QString& s : syms) {
        const double q = pos.value(s, 0.0);
        if (std::abs(q) > eps)
            parts.append(htmlEsc(s) + QStringLiteral(": ") + QString::number(q, 'f', 4));
    }
    if (parts.isEmpty())
        return QStringLiteral("(flat)");
    return parts.join(QStringLiteral(", "));
}

void writeReport(const Backtest::BacktestResult& result,
                        const QString& path,
                        const QMap<QString, QVector<IBComm::HistoricalBar>>& strategyBars,
                        const QDateTime& windowStart,
                        const QDateTime& windowEnd,
                        int rebalanceEveryNBars,
                        double initialCapitalCfg,
                        double maxPositionSharesRisk,
                        const Params& p)
{
    QString     buffer;
    QTextStream html(&buffer);

    const QStringList& syms = p.universe;

    auto barsInWindow = [&](const QString& sym) -> QVector<IBComm::HistoricalBar> {
        QVector<IBComm::HistoricalBar> out;
        const QVector<IBComm::HistoricalBar> raw = strategyBars.value(sym);
        for (const auto& b : raw) {
            if (b.timestamp >= windowStart && b.timestamp <= windowEnd)
                out.append(b);
        }
        return out;
    };

    html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>" << htmlEsc(p.documentTitle)
         << "</title>\n";
    html << "<!-- Generated " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
         << " UTC — rebuild ibtrading_tests after editing; anchor #allocation-tables -->\n";
    html << "<style>body{font-family:system-ui,sans-serif;margin:24px;background:#fafafa;color:#222;}\n";
    html << "table{border-collapse:collapse;background:#fff;margin:12px 0} th,td{border:1px solid #ccc;padding:6px 8px;font-size:12px;}\n";
    html << "th{background:#eee;} .ok{color:#060;} .bad{color:#c00;font-weight:600;} h1{font-size:1.2rem;} "
            "h2{font-size:1.05rem;margin-top:24px;} h3{font-size:1rem;margin-top:16px;color:#333;}\n";
    html << ".kv td:first-child,.kv th:first-child{font-weight:600;width:200px;} svg{background:#fff;border:1px solid #ddd;}\n";
    html << ".infobox{border:2px solid #2e7d32;background:#e8f5e9;padding:14px 16px;margin:16px 0;max-width:920px;line-height:1.45;}"
            "</style></head><body>\n";

    html << "<h1>" << htmlEsc(p.pageHeading) << "</h1>\n";
    html << "<div class=\"infobox\">" << p.primaryInfoboxInnerHtml << "</div>\n";
    html << p.introParagraph1 << "\n";
    html << p.introParagraph2 << "\n";
    if (p.tradingDaysBlurb > 0) {
        html << "<p>Universe size: " << syms.size() << " (deduplicated). Trading days in series: " << p.tradingDaysBlurb
             << ". Momentum ranks all names, keeps top " << p.topN << ", equal-weight rebalance.</p>\n";
    }
    if (!p.introParagraph3.isEmpty())
        html << p.introParagraph3 << "\n";

    html << "<div class=\"infobox\" style=\"border-color:#1565c0;background:#e3f2fd;\"><strong>How this run is configured.</strong> "
            "The portfolio <b>starts with cash only</b> (no positions). The pipeline <b>evaluates every bar close</b> "
            "(momentum and risk can run each session). <b>Rebalancing and execution</b> (trades) occur every "
            "<b>" << rebalanceEveryNBars << "</b> bar closes — with one combined session per calendar day in this "
            "replay, that is about every <b>" << rebalanceEveryNBars << "</b> <b>trading days</b>. "
            "Reported <b>max drawdown</b> in the summary is computed in <code>BacktestMetricsCollector</code> from "
            "<b>end-of-bar mark-to-market equity</b> (ledger snapshots at bar close), not from intraday OHLC lows.</div>\n";

    html << "<h2 id=\"summary-stats\">Summary statistics</h2>\n<table class=\"kv\">\n";
    const QString naCell = QStringLiteral("\u2014"); // em dash — not applicable vs other column

    auto statRow = [&](const char* k, const QString& v) {
        html << "<tr><td>" << k << "</td><td>" << htmlEsc(v) << "</td></tr>\n";
    };
    auto statRow3 = [&](const char* k, const QString& strategyVal, const QString& benchmarkVal) {
        html << "<tr><td>" << k << "</td><td>" << htmlEsc(strategyVal) << "</td><td>" << htmlEsc(benchmarkVal)
             << "</td></tr>\n";
    };

    const QString isoStart = result.startDate.toUTC().toString(Qt::ISODate);
    const QString isoEnd   = result.endDate.toUTC().toString(Qt::ISODate);
    const QString rebText =
        QStringLiteral("every %1 bar closes (~%1 trading days per day in this feed)").arg(rebalanceEveryNBars);

    if (!result.benchmark.symbol.isEmpty()) {
        html << "<tr><th>Metric</th><th>Strategy</th><th>Benchmark (buy-and-hold "
             << htmlEsc(result.benchmark.symbol) << ")</th></tr>\n";

        QString benchFinalCap;
        if (!result.benchmark.equityCurve.isEmpty())
            benchFinalCap = QString::number(result.benchmark.equityCurve.last().portfolioValue, 'f', 2);
        else
            benchFinalCap = QString::number(result.initialCapital * (1.0 + result.benchmark.totalReturn), 'f', 2);

        statRow3("Start", isoStart, isoStart);
        statRow3("End", isoEnd, isoEnd);
        statRow3("Evaluation", QStringLiteral("every bar close (EveryBarClose)"), naCell);
        statRow3("Rebalance / trade", rebText, naCell);
        statRow3("Data quality", dataQualityLabel(result.dataQuality), naCell);
        statRow3("Initial capital", QString::number(result.initialCapital, 'f', 2),
                 QString::number(result.initialCapital, 'f', 2));
        statRow3("Final capital", QString::number(result.finalCapital, 'f', 2), benchFinalCap);
        statRow3("Start price (underlying)", naCell, QString::number(result.benchmark.startPrice, 'f', 4));
        statRow3("End price (underlying)", naCell, QString::number(result.benchmark.endPrice, 'f', 4));
        statRow3("Total return", QString::number(result.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"),
                 QString::number(result.benchmark.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow3("Annualized return", QString::number(result.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"),
                 QString::number(result.benchmark.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow3("Sharpe ratio", QString::number(result.sharpeRatio, 'f', 4),
                 QString::number(result.benchmark.sharpeRatio, 'f', 4));
        statRow3("Max drawdown",
                 QString::number(result.maxDrawdown * 100.0, 'f', 2)
                     + QStringLiteral(" % (from end-of-bar MTM equity; not OHLC intraday low)"),
                 QString::number(result.benchmark.maxDrawdown * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow3("Win rate", QString::number(result.winRate * 100.0, 'f', 2) + QStringLiteral(" %"), naCell);
        statRow3("Total trades (fills)", QString::number(result.totalTrades), naCell);
        statRow3("Equity snapshots", QString::number(result.equityCurve.size()),
                 QString::number(result.benchmark.equityCurve.size()));
        statRow3("Alpha vs benchmark (ann.)",
                 QString::number(result.alphaVsBenchmark * 100.0, 'f', 2) + QStringLiteral(" %"), naCell);
    } else {
        statRow("Start", isoStart);
        statRow("End", isoEnd);
        statRow("Evaluation", QStringLiteral("every bar close (EveryBarClose)"));
        statRow("Rebalance / trade", rebText);
        statRow("Data quality", dataQualityLabel(result.dataQuality));
        statRow("Initial capital", QString::number(result.initialCapital, 'f', 2));
        statRow("Final capital", QString::number(result.finalCapital, 'f', 2));
        statRow("Total return", QString::number(result.totalReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow("Annualized return", QString::number(result.annualizedReturn * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow("Sharpe ratio", QString::number(result.sharpeRatio, 'f', 4));
        statRow("Max drawdown",
                QString::number(result.maxDrawdown * 100.0, 'f', 2)
                    + QStringLiteral(" % (from end-of-bar MTM equity; not OHLC intraday low)"));
        statRow("Win rate", QString::number(result.winRate * 100.0, 'f', 2) + QStringLiteral(" %"));
        statRow("Total trades (fills)", QString::number(result.totalTrades));
        statRow("Equity snapshots", QString::number(result.equityCurve.size()));
    }
    html << "</table>\n";

    html << "<p style=\"margin:10px 0 16px 0;font-size:14px;\"><a href=\"#allocation-tables\"><b>↓ Rebalance "
            "schedule &amp; position allocations</b></a> — starts with cash; scheduled dates vs holdings after "
            "each trade batch.</p>\n";

    QVector<Backtest::FilledOrder> tradesForReport = result.tradeLog;
    std::sort(tradesForReport.begin(), tradesForReport.end(),
              [](const Backtest::FilledOrder& a, const Backtest::FilledOrder& b) {
                  if (a.timestamp != b.timestamp)
                      return a.timestamp < b.timestamp;
                  return a.symbol < b.symbol;
              });

    const QDateTime firstBuy = firstLongBuyTime(tradesForReport);
    const bool      firstFillSell =
        !tradesForReport.isEmpty() && tradesForReport.first().quantity < 0.0;
    html << "<h2 id=\"allocation-tables\">Rebalance &amp; allocations</h2>\n";
    html << "<h3>First buy vs first fills</h3>\n";
    html << "<p>";
    if (firstFillSell) {
        html << "The <b>first</b> fill in chronological order is a <b>sell</b> (negative quantity). "
                "That usually means exiting a prior long or a legacy delta-encoded model row; the native "
                "target pipeline uses absolute targets and does not sell from flat unless you hold the name.</p>\n";
    }
    if (firstBuy.isValid()) {
        html << "First <b>buy</b> fill (positive quantity) is at <b>"
             << htmlEsc(firstBuy.toUTC().toString(Qt::ISODate))
             << "</b> UTC. Earlier sells still reduce longs or add shorts depending on prior holdings.</p>\n";
    } else {
        html << "No buy fills were recorded in this run.</p>\n";
    }

    html << "<h3>Risk limits (this test)</h3>\n<table class=\"kv\">\n";
    statRow("Max position size (per name)",
            QString::number(maxPositionSharesRisk, 'f', 0)
                + QStringLiteral(" shares (max-position-risk block)"));
    statRow("Strategy allocated capital", QString::number(initialCapitalCfg, 'f', 2));
    html << "</table>\n";
    html << "<p style=\"font-size:12px;color:#444\">Equal-weight target for each of the top-" << p.topN
         << " names is "
            "<code>floor((allocatedCapital/" << p.topN << ") / price)</code> shares. Rebalance builds absolute targets; "
            "execution sells (liquidations) are ordered before buys. If a fill is below the equal-weight floor while still buying, "
            "the <b>max-position-risk</b> block may have capped the size. This report does not log per-order "
            "risk audit lines yet.</p>\n";

    const QVector<IBComm::HistoricalBar> scheduleBars = barsForSchedule(strategyBars, syms);
    html << "<h3>Scheduled rebalance sessions (every " << rebalanceEveryNBars << " bars)</h3>\n";
    html << "<p style=\"font-size:12px;color:#444\">Uses the first symbol’s daily bar series in range. "
            "Bar index 0 is the first session in the loaded history; then every " << rebalanceEveryNBars
         << " bars.</p>\n";
    html << "<table><tr><th>#</th><th>Bar index</th><th>Session date (UTC)</th></tr>\n";
    if (!scheduleBars.isEmpty()) {
        int idx = 0;
        for (int i = 0; i < scheduleBars.size(); i += rebalanceEveryNBars) {
            html << "<tr><td>" << idx << "</td><td>" << i << "</td><td>"
                 << htmlEsc(scheduleBars[i].timestamp.toUTC().toString(Qt::ISODate)) << "</td></tr>\n";
            ++idx;
        }
    } else {
        html << "<tr><td colspan=\"3\">(no bars)</td></tr>\n";
    }
    html << "</table>\n";

    html << "<h3>Observed holdings after each trade batch (same timestamp = one rebalance)</h3>\n";
    html << "<p style=\"font-size:12px;color:#444\">Rows group fills with the same timestamp (one rebalance "
            "event). Cumulative net shares are listed <b>only where non-zero</b> (negative = short). "
            "If the portfolio is flat, the cell shows <code>(flat)</code>.</p>\n";
    html << "<table><tr><th>#</th><th>Time (UTC)</th><th>Non-zero net shares (by symbol)</th></tr>\n";
    {
        QMap<QString, double> pos;
        int                    batchIdx = 0;
        for (int i = 0; i < tradesForReport.size();) {
            const QDateTime ts = tradesForReport[i].timestamp;
            int               j = i;
            while (j < tradesForReport.size() && tradesForReport[j].timestamp == ts)
                ++j;
            for (int k = i; k < j; ++k) {
                const QString u = tradesForReport[k].symbol.trimmed().toUpper();
                pos[u] = pos.value(u, 0.0) + tradesForReport[k].quantity;
            }
            html << "<tr><td>" << batchIdx << "</td><td>" << htmlEsc(ts.toUTC().toString(Qt::ISODate))
                 << "</td><td>" << holdingsNonZeroListHtml(syms, pos) << "</td></tr>\n";
            ++batchIdx;
            i = j;
        }
    }
    html << "</table>\n";

    const bool overlayCharts = !result.equityCurve.isEmpty() && !result.benchmark.equityCurve.isEmpty();
    if (overlayCharts) {
        writeSvgStrategyVsBenchmark(html, QStringLiteral("Cumulative P&L — strategy vs buy-and-hold benchmark"),
                                    result.equityCurve, result.benchmark.equityCurve, result.benchmark.symbol,
                                    result.initialCapital, true);
    } else if (!result.equityCurve.isEmpty()) {
        writeSvgStrategyVsBenchmark(html, QStringLiteral("Cumulative P&L — strategy"),
                                    result.equityCurve, {}, QString(), result.initialCapital, true);
    }

    html << "<h2>Trades + historical OHLC (from cache)</h2>\n";

    if (p.fullOhlcGridPerTrade) {
        html << "<table><tr><th>Time (UTC)</th><th>Symbol</th><th>Qty</th><th>Fill</th>";
        for (const QString& sym : syms) {
            html << "<th>" << htmlEsc(sym) << " O</th><th>" << htmlEsc(sym) << " C</th>";
        }
        html << "<th>|Fill − histC| (traded)</th>";
        if (p.includeCorrelationColumn)
            html << "<th>Correlation</th>";
        html << "</tr>\n";

        for (const auto& t : tradesForReport) {
            const QString u = t.symbol.trimmed().toUpper();
            QString       cells;
            double        histC = 0.0;
            for (const QString& sym : syms) {
                const QVector<IBComm::HistoricalBar> all = strategyBars.value(sym);
                const IBComm::HistoricalBar* pb = Backtest::barForSessionDay(all, t.timestamp);
                const double o = pb ? pb->open : 0.0;
                const double c = pb ? pb->close : 0.0;
                cells += QStringLiteral("<td>%1</td><td>%2</td>")
                             .arg(o, 0, 'f', 4)
                             .arg(c, 0, 'f', 4);
                if (sym == u && pb)
                    histC = pb->close;
            }
            const double diff = std::abs(t.fillPrice - histC);
            const double tol = std::max(0.05, 2e-3 * std::abs(histC));
            const bool ok = (histC > 0.0) && (diff <= tol);
            const QString cls = ok ? QStringLiteral("ok") : QStringLiteral("bad");
            html << "<tr><td>" << htmlEsc(t.timestamp.toUTC().toString(Qt::ISODate)) << "</td><td>" << htmlEsc(u)
                 << "</td><td>" << QString::number(t.quantity, 'f', 4) << "</td><td>" << QString::number(t.fillPrice, 'f', 6)
                 << "</td>" << cells << "<td class=\"" << cls << "\">" << QString::number(diff, 'f', 6) << "</td>";
            if (p.includeCorrelationColumn)
                html << "<td>" << htmlEsc(t.correlationId) << "</td>";
            html << "</tr>\n";
        }
        html << "</table>\n";
    } else {
        html << "<p style=\"font-size:12px;color:#444\">Large universe: one <b>Hist O/C</b> pair for the <b>traded</b> symbol only "
                "(full per-name grid omitted).</p>\n";
        html << "<table><tr><th>Time (UTC)</th><th>Symbol</th><th>Qty</th><th>Fill</th>"
                "<th>Hist O</th><th>Hist C</th><th>|Fill − histC| (traded)</th>";
        if (p.includeCorrelationColumn)
            html << "<th>Correlation</th>";
        html << "</tr>\n";
        for (const auto& t : tradesForReport) {
            const QString u = t.symbol.trimmed().toUpper();
            const QVector<IBComm::HistoricalBar> all = strategyBars.value(u);
            const IBComm::HistoricalBar* pb = Backtest::barForSessionDay(all, t.timestamp);
            const double o = pb ? pb->open : 0.0;
            const double c = pb ? pb->close : 0.0;
            const double histC = pb ? pb->close : 0.0;
            const double diff = std::abs(t.fillPrice - histC);
            const double tol = std::max(0.05, 2e-3 * std::abs(histC));
            const bool ok = (histC > 0.0) && (diff <= tol);
            const QString cls = ok ? QStringLiteral("ok") : QStringLiteral("bad");
            html << "<tr><td>" << htmlEsc(t.timestamp.toUTC().toString(Qt::ISODate)) << "</td><td>"
                 << htmlEsc(u) << "</td><td>" << QString::number(t.quantity, 'f', 4) << "</td><td>"
                 << QString::number(t.fillPrice, 'f', 6) << "</td><td>" << QString::number(o, 'f', 4) << "</td><td>"
                 << QString::number(c, 'f', 4) << "</td><td class=\"" << cls << "\">" << QString::number(diff, 'f', 6)
                 << "</td>";
            if (p.includeCorrelationColumn)
                html << "<td>" << htmlEsc(t.correlationId) << "</td>";
            html << "</tr>\n";
        }
        html << "</table>\n";
    }

    html << "<h2>Price (close) + entries / exits</h2>\n";
    html << "<p style=\"font-size:12px;color:#444\">Markers use the same bar session as the table.</p>\n";
    for (const QString& sym : syms) {
        html << "<h3>" << htmlEsc(sym) << "</h3>\n";
        html << svgCloseChart(sym, barsInWindow(sym), result.tradeLog);
    }

    html << "</body></html>\n";
    html.flush();

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "MomentumSemanticValidationHtml::writeReport: cannot open" << path;
        return;
    }
    const QByteArray utf8 = buffer.toUtf8();
    const qint64     n  = f.write(utf8);
    f.close();
    if (n != utf8.size())
        qWarning() << "MomentumSemanticValidationHtml::writeReport: short write" << n << "expected" << utf8.size()
                   << path;
}

} // namespace MomentumSemanticValidationHtml

