#include "SharedUI/TradingChartTheme.h"

#include "MainSystem/ThemePalette.h"

#include <QAbstractAxis>
#include <QBarCategoryAxis>
#include <QBrush>
#include <QChart>
#include <QDateTimeAxis>
#include <QFont>
#include <QLegend>
#include <QPen>
#include <QValueAxis>
#include <QtGlobal>

namespace TradingChartTheme {

namespace {

QColor fromHex(const char* hex)
{
    return QColor(QString::fromLatin1(hex));
}

void styleAxis(QAbstractAxis* axis)
{
    if (!axis)
        return;

    const QColor grid = fromHex(UiTheme::kChartGrid);
    const QColor axisLine = fromHex(UiTheme::kBorderSubtle);
    const QColor labels = fromHex(UiTheme::kTextSecondary);

    axis->setLabelsColor(labels);
    axis->setTitleBrush(QBrush(labels));
    axis->setLinePenColor(axisLine);

    // Ordinal bar index: one vertical grid line per category — too dense; keep Y grid only.
    if (auto* ca = qobject_cast<QBarCategoryAxis*>(axis)) {
        ca->setGridLineVisible(false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        // Default true → long category strings become "..." with almost no visible text.
        ca->setTruncateLabels(false);
#endif
        QFont lf = ca->labelsFont();
        if (lf.pointSizeF() > 0.0 && lf.pointSizeF() > 8.0)
            lf.setPointSizeF(qMax(8.0, lf.pointSizeF() - 1.0));
        ca->setLabelsFont(lf);
    }

    if (auto* va = qobject_cast<QValueAxis*>(axis)) {
        va->setGridLineColor(grid);
        va->setMinorGridLineColor(grid.darker(120));
    }
    if (auto* da = qobject_cast<QDateTimeAxis*>(axis)) {
        da->setGridLineColor(grid);
    }
}

} // namespace

QColor plotBackgroundColor()
{
    return fromHex(UiTheme::kChartPlotBg);
}

QColor gridLineColor()
{
    return fromHex(UiTheme::kChartGrid);
}

QColor axisLabelColor()
{
    return fromHex(UiTheme::kTextSecondary);
}

QColor candleBullColor()
{
    return fromHex(UiTheme::kChartBull);
}

QColor candleBearColor()
{
    return fromHex(UiTheme::kChartBear);
}

void applyDarkTheme(QChart* chart)
{
    if (!chart)
        return;

    chart->setTheme(QChart::ChartThemeDark);
    chart->setBackgroundRoundness(0);
    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(QBrush(fromHex(UiTheme::kBgChrome)));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotBackgroundColor()));

    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(qMax(10, titleFont.pointSize()));
    chart->setTitleFont(titleFont);
    chart->setTitleBrush(QBrush(fromHex(UiTheme::kTextPrimary)));

    if (QLegend* leg = chart->legend()) {
        leg->setLabelColor(fromHex(UiTheme::kTextSecondary));
        leg->setBrush(QBrush(fromHex(UiTheme::kBgChrome)));
        leg->setPen(QPen(fromHex(UiTheme::kBorderSubtle)));
    }

    const QList<QAbstractAxis*> axes = chart->axes();
    for (QAbstractAxis* a : axes)
        styleAxis(a);
}

} // namespace TradingChartTheme
