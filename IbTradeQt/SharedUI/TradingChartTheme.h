#pragma once

#include <QColor>

class QChart;
class QDateTimeAxis;
class QValueAxis;

namespace TradingChartTheme {

void applyDarkTheme(QChart* chart);

QColor plotBackgroundColor();
QColor gridLineColor();
QColor axisLabelColor();
QColor candleBullColor();
QColor candleBearColor();

} // namespace TradingChartTheme
