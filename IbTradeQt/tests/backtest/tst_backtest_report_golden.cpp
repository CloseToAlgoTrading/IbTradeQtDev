#include "backtest/tst_backtest_report_golden.h"

#include "Backtest/BacktestHtmlTemplateRenderer.h"
#include "Backtest/BacktestReportModel.h"
#include "Backtest/BacktestReportModelBuilder.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestReportContext.h"
#include "Backtest/LedgerSnapshot.h"
#include "Backtest/BacktestStatisticsCalculator.h"

#include <QtTest>

using namespace Backtest;

void TestBacktestReportGolden::reportModelJson_includesExtendedSummaryKeys()
{
    BacktestResult r;
    r.initialCapital = 100000.0;
    r.finalCapital   = 100000.0;
    LedgerSnapshot s;
    s.portfolioValue = 100000.0;
    s.cash             = 50000.0;
    r.equityCurve.append(s);
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    BacktestReportContext        ctx;
    BacktestReportModelBuilder   b;
    const BacktestReportModel      m = b.build(r, st, ctx);
    const QJsonObject              summary =
        m.sections.value(QStringLiteral("summary")).toObject();
    QVERIFY(summary.contains(QStringLiteral("averageExposurePct")));
    QVERIFY(summary.contains(QStringLiteral("turnoverAnnualized")));
}

void TestBacktestReportGolden::templateRenderer_replacesPlaceholder()
{
    QJsonObject flat;
    flat[QStringLiteral("title")] = QStringLiteral("X");
    const QString html = QStringLiteral("<html><body>{{title}}</body></html>");
    const QString out  = renderHtmlTemplate(html, flat);
    QVERIFY(out.contains(QStringLiteral("X")));
    QVERIFY(!out.contains(QStringLiteral("{{title}}")));
}

void TestBacktestReportGolden::reportModelJson_hasGeneratorVersion()
{
    BacktestResult r;
    r.initialCapital = 1.0;
    r.finalCapital   = 1.0;
    BacktestStatisticsCalculator calc;
    const BacktestStatistics     st = calc.compute(r);
    BacktestReportContext        ctx;
    BacktestReportModelBuilder   b;
    const BacktestReportModel      m = b.build(r, st, ctx);
    const QJsonObject              o = m.toJson(QStringLiteral("test"));
    QCOMPARE(o.value(QStringLiteral("schemaVersion")).toInt(), 1);
    QCOMPARE(o.value(QStringLiteral("generatorVersion")).toString(), QStringLiteral("test"));
}
