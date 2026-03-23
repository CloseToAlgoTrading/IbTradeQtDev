#ifndef TST_MOMENTUM_STRATEGY_E2E_H
#define TST_MOMENTUM_STRATEGY_E2E_H

// End-to-end backtest: fixed static-list universe (~101 symbols, deduped), semantic momentum
// scores every name in that universe then keeps top 5 (topN), equal-weight rebalance,
// max-position risk (25% notional heuristic via share cap), Yahoo bars from mock HTTP,
// HistoricalDataManager cache for IHistoricalRead. Writes an HTML report (path printed to qInfo).

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QDateTime>
#include <QTimeZone>
#include <QDir>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QTemporaryFile>
#include <QUuid>
#include <algorithm>
#include <cmath>

#include "backtest/tst_yahoo_backtest.h"

#include "Backtest/BacktestSession.h"
#include "Backtest/BacktestConfig.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/HistoricalDataManager.h"
#include "DB/dbquery.h"

namespace {

QStringList momentumE2eUniverseSymbols()
{
    // Same symbol list for backtest universe, static-list selection, and momentum input (all names
    // ranked; alpha topN=5). INTU appears twice in the source list; dedupe yields 101 symbols.
    const QString raw = QStringLiteral(
        "AAPL,MSFT,NVDA,AMZN,GOOGL,GOOG,META,TSLA,BRK.B,AVGO,JPM,LLY,UNH,XOM,V,PG,COST,MA,HD,MRK,"
        "ABBV,CVX,PEP,KO,BAC,WMT,TMO,CRM,ORCL,ACN,MCD,DHR,CSCO,LIN,ABT,NFLX,AMD,WFC,DIS,TXN,VZ,PM,"
        "RTX,HON,IBM,QCOM,AMGN,CAT,INTU,GS,SBUX,NOW,INTU,BA,GE,SPGI,ISRG,BKNG,ADP,MDT,DE,PLD,LMT,"
        "ADI,CB,MMC,SYK,GILD,CI,ELV,MO,UPS,T,BLK,AMAT,AXP,MDLZ,TMUS,SCHW,PGR,CME,ZTS,C,MU,REGN,USB,"
        "SO,EOG,DUKE,CSX,NEE,FIS,HCA,AON,ETN,GM,FDX,ITW,KLAC,PANW,SNPS,CDNS");
    QStringList out;
    QSet<QString> seen;
    for (const QString& part : raw.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString s = part.trimmed().toUpper();
        if (s.isEmpty() || seen.contains(s))
            continue;
        seen.insert(s);
        out.append(s);
    }
    return out;
}

static void createHistoricalBarsTable(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery     q(db);
    QVERIFY(q.exec(QLatin1String(CREATE_TABLE_HISTORICAL_BARS)));
}

static QString htmlEscape(const QString& s)
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

static void writeMomentumE2eHtmlReport(const Backtest::BacktestResult& result,
                                       const QString& path,
                                       const QStringList& universe,
                                       int tradingDays,
                                       int evalEveryNBars)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "writeMomentumE2eHtmlReport: cannot open" << path;
        return;
    }
    QTextStream html(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    html.setEncoding(QStringConverter::Utf8);
#endif

    html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Momentum strategy E2E backtest</title>\n";
    html << "<style>body{font-family:system-ui,Segoe UI,sans-serif;margin:24px;background:#fafafa;color:#222;}\n";
    html << "table{border-collapse:collapse;margin:16px 0;background:#fff;}\n";
    html << "th,td{border:1px solid #ccc;padding:6px 10px;text-align:left;font-size:13px;}\n";
    html << "th{background:#eee;} h1{font-size:1.35rem;} h2{font-size:1.1rem;margin-top:28px;}\n";
    html << ".kv td:first-child{font-weight:600;width:220px;} svg{background:#fff;border:1px solid #ddd;}\n";
    html << "</style></head><body>\n";
    html << "<h1>Momentum strategy — end-to-end backtest report</h1>\n";
    html << "<p>Universe size: " << universe.size() << " (deduplicated). Synthetic Yahoo bars, mock HTTP. "
         << "Trading days in series: " << tradingDays << ". Evaluation / rebalance cadence: every "
         << evalEveryNBars << " bar closes.</p>\n";

    html << "<h2>Summary statistics</h2>\n<table class=\"kv\">\n";
    auto row = [&](const char* k, const QString& v) {
        html << "<tr><td>" << k << "</td><td>" << htmlEscape(v) << "</td></tr>\n";
    };
    row("Start", result.startDate.toUTC().toString(Qt::ISODate));
    row("End", result.endDate.toUTC().toString(Qt::ISODate));
    row("Initial capital", QString::number(result.initialCapital, 'f', 2));
    row("Final capital", QString::number(result.finalCapital, 'f', 2));
    row("Total return", QString::number(result.totalReturn * 100.0, 'f', 2) + " %");
    row("Annualized return", QString::number(result.annualizedReturn * 100.0, 'f', 2) + " %");
    row("Sharpe ratio", QString::number(result.sharpeRatio, 'f', 4));
    row("Max drawdown", QString::number(result.maxDrawdown * 100.0, 'f', 2) + " %");
    row("Win rate", QString::number(result.winRate * 100.0, 'f', 2) + " %");
    row("Total trades", QString::number(result.totalTrades));
    html << "</table>\n";

    // Equity curve SVG (normalize to 0–100 width/height for path)
    if (!result.equityCurve.isEmpty()) {
        const int W = 900;
        const int H = 280;
        const int pad = 20;
        double minEq = result.equityCurve.first().portfolioValue;
        double maxEq = minEq;
        for (const auto& pt : result.equityCurve) {
            minEq = std::min(minEq, pt.portfolioValue);
            maxEq = std::max(maxEq, pt.portfolioValue);
        }
        if (qFuzzyCompare(maxEq, minEq)) {
            maxEq = minEq + 1.0;
        }
        html << "<h2>Equity curve (portfolio value)</h2>\n<svg width=\"" << W << "\" height=\"" << H
             << "\" viewBox=\"0 0 " << W << " " << H << "\">\n";
        html << "<rect x=\"0\" y=\"0\" width=\"" << W << "\" height=\"" << H << "\" fill=\"#fafafa\"/>\n";
        QString pathD = QStringLiteral("M ");
        const int n = result.equityCurve.size();
        for (int i = 0; i < n; ++i) {
            const double x = pad + (n <= 1 ? 0 : double(i) / double(n - 1)) * (W - 2 * pad);
            const double y = pad + (1.0 - (result.equityCurve[i].portfolioValue - minEq) / (maxEq - minEq))
                                  * (H - 2 * pad);
            pathD += QString::number(x, 'f', 2) + QLatin1Char(' ') + QString::number(y, 'f', 2);
            if (i < n - 1)
                pathD += QStringLiteral(" L ");
        }
        html << "<path d=\"" << htmlEscape(pathD) << "\" fill=\"none\" stroke=\"#1a73e8\" stroke-width=\"2\"/>\n";
        html << "</svg>\n";
    }

    html << "<h2>Trades</h2>\n<table><tr><th>Time</th><th>Symbol</th><th>Qty</th><th>Market (ref)</th>"
            "<th>Fill price</th><th>Correlation</th></tr>\n";
    for (const auto& t : result.tradeLog) {
        const double mkt = t.marketPrice > 0.0 ? t.marketPrice : t.fillPrice;
        html << "<tr><td>" << htmlEscape(t.timestamp.toUTC().toString(Qt::ISODate)) << "</td><td>"
             << htmlEscape(t.symbol) << "</td><td>" << QString::number(t.quantity, 'f', 4) << "</td><td>"
             << QString::number(mkt, 'f', 4) << "</td><td>" << QString::number(t.fillPrice, 'f', 4)
             << "</td><td>" << htmlEscape(t.correlationId) << "</td></tr>\n";
    }
    html << "</table>\n";

    // Holdings over time: replay trades at each equity snapshot timestamp
    html << "<h2>Holdings history (from trade log at equity snapshot times)</h2>\n";
    html << "<p>Positions are reconstructed from fills; cash is not shown per row (see ledger snapshots in metrics).</p>\n";

    QVector<QPair<QDateTime, QHash<QString, double>>> snapshots;
    snapshots.reserve(result.equityCurve.size());
    int tradeIdx = 0;
    const int nTrades = result.tradeLog.size();
    QHash<QString, double> pos;
    for (const auto& snap : result.equityCurve) {
        while (tradeIdx < nTrades && result.tradeLog[tradeIdx].timestamp <= snap.timestamp) {
            const auto& tr = result.tradeLog[tradeIdx];
            pos[tr.symbol.toUpper()] = pos.value(tr.symbol.toUpper(), 0.0) + tr.quantity;
            ++tradeIdx;
        }
        snapshots.append({snap.timestamp, pos});
    }

    html << "<table><tr><th>Time</th><th>Positions (symbol → shares)</th></tr>\n";
    for (const auto& pr : snapshots) {
        QStringList cells;
        for (auto it = pr.second.constBegin(); it != pr.second.constEnd(); ++it) {
            if (std::abs(it.value()) < 1e-9)
                continue;
            cells.append(it.key() + QLatin1Char('=') + QString::number(it.value(), 'f', 2));
        }
        std::sort(cells.begin(), cells.end());
        html << "<tr><td>" << htmlEscape(pr.first.toUTC().toString(Qt::ISODate)) << "</td><td>"
             << htmlEscape(cells.join(QStringLiteral("; "))) << "</td></tr>\n";
    }
    html << "</table>\n";

    html << "</body></html>\n";
}

} // namespace

// ---------------------------------------------------------------------------
class TestMomentumStrategyE2E : public QObject {
    Q_OBJECT
private slots:

    void momentum_top5_equalWeight_risk25_rebal22_generatesReport()
    {
        const bool fullSpec = qgetenv("IBTRADING_MOMENTUM_E2E_FULL") == "1";
        QStringList universe = momentumE2eUniverseSymbols();
        QCOMPARE(universe.size(), 101);

        int tradingDays = fullSpec ? 252 * 10 : 400;
        {
            bool         ok = false;
            const int    v  = qgetenv("IBTRADING_MOMENTUM_E2E_TRADING_DAYS").toInt(&ok);
            if (ok && v > 0)
                tradingDays = v;
        }
        const int evalEvery   = 22;

        const QDate endCal = QDateTime::currentDateTimeUtc().date();
        QDate       startCal = endCal.addDays(-static_cast<int>(tradingDays * 1.45) - 10);
        // Align to a weekday
        while (startCal.dayOfWeek() >= 6)
            startCal = startCal.addDays(1);

        // Extra year of synthetic history so momentum (lookbackYears=1, period=20) always has
        // enough Day1 bars in the cache before the first evaluation; matches prefetch window.
        const QDate dataStart = startCal.addYears(-1);
        const int   barCount  = tradingDays + 300;

        const QDateTime startDate(QDateTime(dataStart, QTime(0, 0), QTimeZone::utc()));
        const QDateTime endDate(endCal, QTime(23, 59, 59), QTimeZone::utc());

        auto* mgr = new MockNetworkAccessManager();
        for (int i = 0; i < universe.size(); ++i) {
            const QString& sym = universe[i];
            const double   p0  = 20.0 + double((i * 17) % 180);
            const quint32  seed = quint32(101 + i * 17);
            auto           bars =
                generateDailyBars(sym, dataStart, barCount, p0, 0.0004, 0.018, seed);
            mgr->addSymbolResponse(sym, historicalBarsToYahooJson(sym, bars));
        }

        const QString conn = QStringLiteral("mom_e2e_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QTemporaryFile dbFile;
        dbFile.setAutoRemove(true);
        QVERIFY(dbFile.open());
        dbFile.close();

        std::unique_ptr<Backtest::HistoricalDataManager> histMgr;

        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
            db.setDatabaseName(dbFile.fileName());
            QVERIFY(db.open());
            createHistoricalBarsTable(conn);

            histMgr = std::make_unique<Backtest::HistoricalDataManager>(conn, mgr);
            QString   refreshed;
            QMap<QString, QVector<IBComm::HistoricalBar>> strategyBars =
                histMgr->getBarsMulti(universe, QStringLiteral("Day1"), QStringLiteral("yahoo"),
                                      startDate, endDate, &refreshed, {});
            QVERIFY2(!strategyBars.isEmpty(), "Expected pre-fetch to populate strategy bars");
            for (const QString& s : universe) {
                const int n = strategyBars.value(s).size();
                QVERIFY2(n > 30,
                         qPrintable(QStringLiteral("Prefetched Day1 bars missing or too few for ")
                                    + s + QStringLiteral(" (got ") + QString::number(n) + QLatin1Char(')')));
            }

            const double initialCapital = 100'000.0;
            // Heuristic: 25% of capital in notional → share cap assuming price floor ~ $25
            const double maxShares =
                std::floor(initialCapital * 0.25 / 25.0);

            // Selection model: same full list as strategy universe (intersection with universe is
            // the full set); momentum-alpha then ranks all passed symbols and emits top 5.
            QJsonArray selSyms;
            for (const QString& s : universe)
                selSyms.append(s);

            QJsonObject pipeline;
            pipeline[QStringLiteral("semanticPipeline")]      = true;
            pipeline[QStringLiteral("semanticModelRebalance")] = true;
            pipeline[QStringLiteral("strategyAllocatedCapital")] = initialCapital;

            QJsonObject sel;
            sel[QStringLiteral("blockId")] = QStringLiteral("static-list-selection");
            sel[QStringLiteral("config")]  = QJsonObject{{QStringLiteral("symbols"), selSyms}};
            pipeline[QStringLiteral("selection")] = sel;

            QJsonObject alphaCfg;
            alphaCfg[QStringLiteral("period")]         = 20;
            alphaCfg[QStringLiteral("threshold")]     = 0.001;
            alphaCfg[QStringLiteral("topN")]            = 5;
            alphaCfg[QStringLiteral("lookbackYears")]   = 1;
            alphaCfg[QStringLiteral("resolution")]      = QStringLiteral("Day1");
            alphaCfg[QStringLiteral("dataSourceId")]  = QStringLiteral("yahoo");

            pipeline[QStringLiteral("alphas")] =
                QJsonArray{QJsonObject{{QStringLiteral("blockId"), QStringLiteral("momentum-alpha")},
                                       {QStringLiteral("config"), alphaCfg}}};

            pipeline[QStringLiteral("rebalance")] =
                QJsonObject{{QStringLiteral("blockId"), QStringLiteral("simple-rebalance")},
                            {QStringLiteral("config"),
                             QJsonObject{{QStringLiteral("equalWeight"), true}}}};

            pipeline[QStringLiteral("risks")] = QJsonArray{
                QJsonObject{{QStringLiteral("blockId"), QStringLiteral("max-position-risk")},
                            {QStringLiteral("config"),
                             QJsonObject{{QStringLiteral("maxPositionSize"), maxShares},
                                          {QStringLiteral("maxTotalExposure"), 1.0e12}}}}};

            pipeline[QStringLiteral("execution")] =
                QJsonObject{{QStringLiteral("blockId"), QStringLiteral("market-order-execution")},
                            {QStringLiteral("config"), QJsonObject{}}};
            pipeline[QStringLiteral("mergePolicy")] = QString();

            QJsonObject rp;
            rp[QStringLiteral("evaluationMode")]    = QStringLiteral("EveryNBars");
            rp[QStringLiteral("evaluationIntervalN")] = evalEvery;
            rp[QStringLiteral("rebalanceMode")]       = QStringLiteral("Immediate");
            rp[QStringLiteral("accumulateAlphaSignals")] = false;
            pipeline[QStringLiteral("runtimePolicy")] = rp;

            Backtest::BacktestConfig config;
            config.startDate      = startDate;
            config.endDate        = endDate;
            config.symbols        = universe;
            config.dataSourceId   = QStringLiteral("yahoo");
            config.resolution     = Backtest::BarResolution::Day1;
            config.fillModel      = Backtest::FillModelType::MidPrice;
            config.fillTiming     = Backtest::FillTiming::SignalOnClose_FillAtClose;
            config.initialCapital = initialCapital;
            config.benchmarkSymbol.clear();

            Backtest::BacktestSession session(config);
            session.setPipelineConfig(pipeline);
            session.setYahooNetworkAccessManager(mgr);
            session.setPreloadedBars(strategyBars);
            session.setHistoricalDataManager(histMgr.get());

            Backtest::BacktestResult result;
            QString                  err;
            bool                     ok = false;
            connect(&session, &Backtest::BacktestSession::finished,
                    [&](const Backtest::BacktestResult& r) {
                        result = r;
                        ok     = true;
                    });
            connect(&session, &Backtest::BacktestSession::failed, [&](const QString& e) { err = e; });

            session.run();

            QVERIFY2(ok, qPrintable(QStringLiteral("Session failed: ") + err));
            QVERIFY2(result.equityCurve.size() > 10,
                     qPrintable(QStringLiteral("Equity points: ") + QString::number(result.equityCurve.size())));
            QVERIFY2(result.totalTrades > 0, "Expected at least one fill");
            QVERIFY2(result.totalTrades >= 5,
                     qPrintable(QStringLiteral("Equal-weight top-5 should produce multiple fills on first "
                                               "rebalance; got totalTrades=")
                                + QString::number(result.totalTrades)));
            QCOMPARE(result.initialCapital, initialCapital);
            QVERIFY2(qIsFinite(result.finalCapital),
                     qPrintable(QStringLiteral("finalCapital not finite: ")
                                + QString::number(result.finalCapital)));

            // Write next to tests/ (SRCDIR) so file:// opens reliably; /tmp is machine-specific
            // and is often empty if tests were not run on this host.
            const QString reportPath =
                QDir(QString(SRCDIR)).absoluteFilePath(QStringLiteral("momentum_e2e_report.html"));
            writeMomentumE2eHtmlReport(result, reportPath, universe, tradingDays, evalEvery);
            qInfo() << "Momentum E2E HTML report (open in browser):" << reportPath;

            QVERIFY(QFile::exists(reportPath));

            if (db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase(conn);
    }
};

#endif // TST_MOMENTUM_STRATEGY_E2E_H
