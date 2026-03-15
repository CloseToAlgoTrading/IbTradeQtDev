#ifndef BACKTEST_BACTESTREPORTWRITER_H
#define BACKTEST_BACTESTREPORTWRITER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <cmath>
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestConfig.h"

namespace Backtest {

// Writes a backtest result to disk as both a self-contained HTML report
// and a plain-text summary.
//
// Usage:
//   BacktestReportWriter writer("/tmp/reports");
//   writer.write(config, result);
//   // produces:
//   //   /tmp/reports/backtest_AMD_NVDA_20150102_20180102.html
//   //   /tmp/reports/backtest_AMD_NVDA_20150102_20180102.txt
class BacktestReportWriter {
public:
    explicit BacktestReportWriter(const QString& outputDir = QDir::currentPath())
        : m_outputDir(outputDir)
    {}

    // Returns the path of the HTML report written, or empty string on error.
    QString write(const BacktestConfig& config, const BacktestResult& result)
    {
        QDir().mkpath(m_outputDir);

        const QString stem = buildStem(config);
        const QString htmlPath = m_outputDir + "/" + stem + ".html";
        const QString txtPath  = m_outputDir + "/" + stem + ".txt";

        writeTxt(txtPath,  config, result);
        writeHtml(htmlPath, config, result);

        return htmlPath;
    }

private:
    // -----------------------------------------------------------------------
    // Plain-text report
    // -----------------------------------------------------------------------
    void writeTxt(const QString& path,
                  const BacktestConfig& config,
                  const BacktestResult& result) const
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&f);

        const QString sep(72, '=');
        const QString dash(72, '-');

        out << sep << "\n";
        out << "  BACKTEST REPORT\n";
        out << sep << "\n\n";

        out << "Strategy config : " << config.strategyConfigPath << "\n";
        out << "Symbols         : " << config.symbols.join(", ") << "\n";
        out << "Period          : " << config.startDate.toString("yyyy-MM-dd")
            << " → " << config.endDate.toString("yyyy-MM-dd") << "\n";
        out << "Data source     : " << config.dataSourceId << "\n";
        out << "Resolution      : Day1\n";
        out << "Initial capital : $" << QString::number(config.initialCapital, 'f', 2) << "\n";
        out << "Fill model      : " << fillModelName(config.fillModel) << "\n";
        out << "Fill timing     : " << fillTimingName(config.fillTiming) << "\n";
        out << "Slippage        : " << config.slippageBps << " bps\n";
        out << "Generated at    : " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";

        out << dash << "\n";
        out << "  STRATEGY PERFORMANCE\n";
        out << dash << "\n";
        out << row("Initial capital",  "$" + QString::number(result.initialCapital, 'f', 2));
        out << row("Final capital",    "$" + QString::number(result.finalCapital,   'f', 2));
        out << row("Total return",     pct(result.totalReturn));
        out << row("Annualised return",pct(result.annualizedReturn));
        out << row("Sharpe ratio",     QString::number(result.sharpeRatio, 'f', 3));
        out << row("Max drawdown",     pct(result.maxDrawdown));
        out << row("Win rate",         pct(result.winRate));
        out << row("Total trades",     QString::number(result.totalTrades));
        out << row("Equity curve pts", QString::number(result.equityCurve.size()));
        out << row("Data quality",     dataQualityName(result.dataQuality));
        out << "\n";

        if (!result.benchmark.symbol.isEmpty()) {
            out << dash << "\n";
            out << "  BENCHMARK (" << result.benchmark.symbol << " — buy-and-hold)\n";
            out << dash << "\n";
            out << row("Start price",      "$" + QString::number(result.benchmark.startPrice, 'f', 2));
            out << row("End price",        "$" + QString::number(result.benchmark.endPrice,   'f', 2));
            out << row("Total return",     pct(result.benchmark.totalReturn));
            out << row("Annualised return",pct(result.benchmark.annualizedReturn));
            out << row("Sharpe ratio",     QString::number(result.benchmark.sharpeRatio, 'f', 3));
            out << row("Max drawdown",     pct(result.benchmark.maxDrawdown));
            out << "\n";

            out << dash << "\n";
            out << "  ALPHA vs BENCHMARK\n";
            out << dash << "\n";
            out << row("Alpha (annualised)", pct(result.alphaVsBenchmark));
            out << "\n";
        }

        if (!result.tradeLog.isEmpty()) {
            out << dash << "\n";
            out << "  TRADE LOG (" << result.tradeLog.size() << " fills)\n";
            out << dash << "\n";
            out << col("Date", 12) << col("Side", 6) << col("Qty", 10)
                << col("Price", 12) << "Symbol\n";
            out << QString(72, '-') << "\n";
            for (const auto& fill : result.tradeLog) {
                const QString side = fill.quantity > 0 ? "BUY" : "SELL";
                out << col(fill.timestamp.toString("yyyy-MM-dd"), 12)
                    << col(side, 6)
                    << col(QString::number(std::abs(fill.quantity), 'f', 0), 10)
                    << col("$" + QString::number(fill.fillPrice, 'f', 2), 12)
                    << fill.symbol << "\n";
            }
            out << "\n";
        }

        out << sep << "\n";
        out << "  END OF REPORT\n";
        out << sep << "\n";
    }

    // -----------------------------------------------------------------------
    // HTML report — self-contained, no external dependencies
    // -----------------------------------------------------------------------
    void writeHtml(const QString& path,
                   const BacktestConfig& config,
                   const BacktestResult& result) const
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream out(&f);

        const QString title = QString("Backtest: %1 | %2 → %3")
            .arg(config.symbols.join("+"))
            .arg(config.startDate.toString("yyyy-MM-dd"))
            .arg(config.endDate.toString("yyyy-MM-dd"));

        out << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>)" << title << R"(</title>
<style>
  :root {
    --bg: #0d1117; --surface: #161b22; --border: #30363d;
    --text: #e6edf3; --muted: #8b949e; --accent: #58a6ff;
    --green: #3fb950; --red: #f85149; --yellow: #d29922;
    --font-mono: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
    --font-sans: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { background: var(--bg); color: var(--text); font-family: var(--font-sans);
         font-size: 14px; line-height: 1.6; padding: 2rem; }
  h1 { font-size: 1.6rem; font-weight: 700; color: var(--accent); margin-bottom: 0.25rem; }
  h2 { font-size: 1rem; font-weight: 600; color: var(--muted); text-transform: uppercase;
       letter-spacing: 0.08em; margin: 2rem 0 0.75rem; border-bottom: 1px solid var(--border);
       padding-bottom: 0.4rem; }
  .meta { color: var(--muted); font-size: 0.85rem; margin-bottom: 2rem; }
  .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 1rem; }
  .card { background: var(--surface); border: 1px solid var(--border); border-radius: 8px;
          padding: 1rem 1.25rem; }
  .card-label { font-size: 0.75rem; color: var(--muted); text-transform: uppercase;
                letter-spacing: 0.06em; margin-bottom: 0.3rem; }
  .card-value { font-size: 1.4rem; font-weight: 700; font-family: var(--font-mono); }
  .pos { color: var(--green); } .neg { color: var(--red); } .neu { color: var(--text); }
  .chart-wrap { background: var(--surface); border: 1px solid var(--border); border-radius: 8px;
                padding: 1.25rem; margin-top: 0.5rem; }
  canvas { width: 100% !important; }
  table { width: 100%; border-collapse: collapse; font-size: 0.85rem; margin-top: 0.5rem; }
  th { text-align: left; padding: 0.5rem 0.75rem; color: var(--muted); font-weight: 600;
       border-bottom: 1px solid var(--border); font-size: 0.75rem; text-transform: uppercase; }
  td { padding: 0.45rem 0.75rem; border-bottom: 1px solid #21262d; font-family: var(--font-mono); }
  tr:hover td { background: #1c2128; }
  .buy { color: var(--green); } .sell { color: var(--red); }
  .badge { display: inline-block; padding: 0.15rem 0.5rem; border-radius: 4px;
           font-size: 0.75rem; font-weight: 600; }
  .badge-green { background: #1a3a1a; color: var(--green); }
  .badge-red   { background: #3a1a1a; color: var(--red); }
  .badge-blue  { background: #1a2a3a; color: var(--accent); }
  footer { margin-top: 3rem; color: var(--muted); font-size: 0.8rem; text-align: center; }
</style>
</head>
<body>
)";

        out << "<h1>" << title << "</h1>\n";
        out << "<p class=\"meta\">"
            << "Data: " << config.dataSourceId << " &nbsp;|&nbsp; "
            << "Fill: " << fillModelName(config.fillModel) << " &nbsp;|&nbsp; "
            << "Timing: " << fillTimingName(config.fillTiming) << " &nbsp;|&nbsp; "
            << "Slippage: " << config.slippageBps << " bps &nbsp;|&nbsp; "
            << "Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << "</p>\n";

        // ---- Strategy KPI cards ----
        out << "<h2>Strategy Performance</h2>\n<div class=\"grid\">\n";
        emitCard(out, "Initial Capital",   "$" + QString::number(result.initialCapital, 'f', 0), "neu");
        emitCard(out, "Final Capital",     "$" + QString::number(result.finalCapital,   'f', 0),
                 result.finalCapital >= result.initialCapital ? "pos" : "neg");
        emitCard(out, "Total Return",      pct(result.totalReturn),
                 result.totalReturn >= 0 ? "pos" : "neg");
        emitCard(out, "Ann. Return",       pct(result.annualizedReturn),
                 result.annualizedReturn >= 0 ? "pos" : "neg");
        emitCard(out, "Sharpe Ratio",      QString::number(result.sharpeRatio, 'f', 2),
                 result.sharpeRatio >= 1.0 ? "pos" : result.sharpeRatio >= 0 ? "neu" : "neg");
        emitCard(out, "Max Drawdown",      pct(result.maxDrawdown), "neg");
        emitCard(out, "Win Rate",          pct(result.winRate),
                 result.winRate >= 0.5 ? "pos" : "neu");
        emitCard(out, "Total Trades",      QString::number(result.totalTrades), "neu");
        out << "</div>\n";

        // ---- Benchmark KPI cards ----
        if (!result.benchmark.symbol.isEmpty()) {
            out << "<h2>Benchmark: " << result.benchmark.symbol << " (buy-and-hold)</h2>\n";
            out << "<div class=\"grid\">\n";
            emitCard(out, "Start Price",   "$" + QString::number(result.benchmark.startPrice, 'f', 2), "neu");
            emitCard(out, "End Price",     "$" + QString::number(result.benchmark.endPrice,   'f', 2),
                     result.benchmark.endPrice >= result.benchmark.startPrice ? "pos" : "neg");
            emitCard(out, "Total Return",  pct(result.benchmark.totalReturn),
                     result.benchmark.totalReturn >= 0 ? "pos" : "neg");
            emitCard(out, "Ann. Return",   pct(result.benchmark.annualizedReturn),
                     result.benchmark.annualizedReturn >= 0 ? "pos" : "neg");
            emitCard(out, "Sharpe Ratio",  QString::number(result.benchmark.sharpeRatio, 'f', 2),
                     result.benchmark.sharpeRatio >= 1.0 ? "pos" : "neu");
            emitCard(out, "Max Drawdown",  pct(result.benchmark.maxDrawdown), "neg");
            emitCard(out, "Alpha (Ann.)",  pct(result.alphaVsBenchmark),
                     result.alphaVsBenchmark >= 0 ? "pos" : "neg");
            out << "</div>\n";
        }

        // ---- Equity curve chart ----
        if (!result.equityCurve.isEmpty()) {
            out << "<h2>Equity Curve</h2>\n";
            out << "<div class=\"chart-wrap\"><canvas id=\"eqChart\" height=\"80\"></canvas></div>\n";
        }

        // ---- Trade log table ----
        if (!result.tradeLog.isEmpty()) {
            out << "<h2>Trade Log (" << result.tradeLog.size() << " fills)</h2>\n";
            out << "<table>\n<thead><tr>"
                << "<th>#</th><th>Date</th><th>Symbol</th><th>Side</th>"
                << "<th>Qty</th><th>Price</th><th>Value</th>"
                << "</tr></thead>\n<tbody>\n";
            int n = 0;
            for (const auto& fill : result.tradeLog) {
                const bool isBuy = fill.quantity > 0;
                const double value = std::abs(fill.quantity) * fill.fillPrice;
                out << "<tr>"
                    << "<td>" << ++n << "</td>"
                    << "<td>" << fill.timestamp.toString("yyyy-MM-dd") << "</td>"
                    << "<td>" << fill.symbol << "</td>"
                    << "<td><span class=\"badge " << (isBuy ? "badge-green" : "badge-red") << "\">"
                    << (isBuy ? "BUY" : "SELL") << "</span></td>"
                    << "<td>" << QString::number(std::abs(fill.quantity), 'f', 0) << "</td>"
                    << "<td>$" << QString::number(fill.fillPrice, 'f', 2) << "</td>"
                    << "<td>$" << QString::number(value, 'f', 0) << "</td>"
                    << "</tr>\n";
            }
            out << "</tbody></table>\n";
        }

        // ---- Inline Chart.js equity curve ----
        if (!result.equityCurve.isEmpty()) {
            out << R"(
<script>
(function() {
  // Inline Chart.js 4.x UMD — loaded from CDN at render time
  var s = document.createElement('script');
  s.src = 'https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js';
  s.onload = function() { renderChart(); };
  document.head.appendChild(s);

  function renderChart() {
    var ctx = document.getElementById('eqChart').getContext('2d');
)";
            // Emit strategy equity curve data
            out << "    var labels = [";
            bool first = true;
            // Downsample to at most 500 points for chart readability
            const int step = std::max(1, static_cast<int>(result.equityCurve.size()) / 500);
            for (int i = 0; i < result.equityCurve.size(); i += step) {
                if (!first) out << ",";
                out << "'" << result.equityCurve[i].timestamp.toString("yyyy-MM-dd") << "'";
                first = false;
            }
            out << "];\n";

            out << "    var stratData = [";
            first = true;
            for (int i = 0; i < result.equityCurve.size(); i += step) {
                if (!first) out << ",";
                out << result.equityCurve[i].portfolioValue;
                first = false;
            }
            out << "];\n";

            // Benchmark equity curve (if available and same length)
            bool hasBm = !result.benchmark.equityCurve.isEmpty();
            if (hasBm) {
                out << "    var bmData = [";
                first = true;
                const int bmStep = std::max(1, static_cast<int>(result.benchmark.equityCurve.size()) / 500);
                for (int i = 0; i < result.benchmark.equityCurve.size(); i += bmStep) {
                    if (!first) out << ",";
                    out << result.benchmark.equityCurve[i].portfolioValue;
                    first = false;
                }
                out << "];\n";
            }

            out << R"(
    var datasets = [{
      label: 'Strategy',
      data: stratData,
      borderColor: '#58a6ff',
      backgroundColor: 'rgba(88,166,255,0.08)',
      borderWidth: 2,
      pointRadius: 0,
      fill: true,
      tension: 0.1
    }];
)";
            if (hasBm) {
                out << R"(
    datasets.push({
      label: ')" << result.benchmark.symbol << R"( (B&H)',
      data: bmData,
      borderColor: '#d29922',
      backgroundColor: 'rgba(210,153,34,0.05)',
      borderWidth: 1.5,
      pointRadius: 0,
      fill: false,
      tension: 0.1
    });
)";
            }

            out << R"(
    new Chart(ctx, {
      type: 'line',
      data: { labels: labels, datasets: datasets },
      options: {
        responsive: true,
        interaction: { mode: 'index', intersect: false },
        plugins: {
          legend: { labels: { color: '#e6edf3' } },
          tooltip: {
            backgroundColor: '#161b22',
            borderColor: '#30363d',
            borderWidth: 1,
            titleColor: '#8b949e',
            bodyColor: '#e6edf3',
            callbacks: {
              label: function(ctx) {
                return ctx.dataset.label + ': $' + ctx.parsed.y.toLocaleString('en-US', {maximumFractionDigits:0});
              }
            }
          }
        },
        scales: {
          x: { ticks: { color: '#8b949e', maxTicksLimit: 12 }, grid: { color: '#21262d' } },
          y: {
            ticks: {
              color: '#8b949e',
              callback: function(v) { return '$' + v.toLocaleString('en-US', {maximumFractionDigits:0}); }
            },
            grid: { color: '#21262d' }
          }
        }
      }
    });
  }
})();
</script>
)";
        }

        out << "<footer>IbTradeQt Backtester &mdash; "
            << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << "</footer>\n</body>\n</html>\n";
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    static void emitCard(QTextStream& out, const QString& label,
                         const QString& value, const QString& cls)
    {
        out << "  <div class=\"card\">\n"
            << "    <div class=\"card-label\">" << label << "</div>\n"
            << "    <div class=\"card-value " << cls << "\">" << value << "</div>\n"
            << "  </div>\n";
    }

    static QString pct(double v) {
        return QString::number(v * 100.0, 'f', 2) + "%";
    }

    // Left-pad a string to a fixed width
    static QString col(const QString& s, int width) {
        return s.leftJustified(width, ' ');
    }
    // One key: value row for the TXT report
    static QString row(const QString& label, const QString& value) {
        return col(label + ":", 24) + value + "\n";
    }

    static QString fillModelName(FillModelType m) {
        switch (m) {
        case FillModelType::Instant:     return "Instant";
        case FillModelType::MidPrice:    return "MidPrice";
        case FillModelType::BidAsk:      return "BidAsk";
        case FillModelType::SlippageBps: return "SlippageBps";
        }
        return "Unknown";
    }

    static QString fillTimingName(FillTiming t) {
        switch (t) {
        case FillTiming::SignalOnClose_FillNextBarOpen: return "SignalOnClose→FillNextOpen";
        case FillTiming::SignalOnTick_FillAtBidAsk:    return "SignalOnTick→FillBidAsk";
        case FillTiming::SignalOnClose_FillAtClose:    return "SignalOnClose→FillAtClose";
        }
        return "Unknown";
    }

    static QString dataQualityName(DataQuality q) {
        switch (q) {
        case DataQuality::RealTicks:      return "RealTicks";
        case DataQuality::SynthesizedOHLC:return "SynthesizedOHLC";
        case DataQuality::DailyBars:      return "DailyBars";
        }
        return "Unknown";
    }

    static QString buildStem(const BacktestConfig& config) {
        return QString("backtest_%1_%2_%3")
            .arg(config.symbols.join("_"))
            .arg(config.startDate.toString("yyyyMMdd"))
            .arg(config.endDate.toString("yyyyMMdd"));
    }

    QString m_outputDir;
};

} // namespace Backtest

#endif // BACKTEST_BACTESTREPORTWRITER_H
