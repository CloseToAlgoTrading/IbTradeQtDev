#ifndef BACKTESTUI_EQUITYCHARTWIDGET_H
#define BACKTESTUI_EQUITYCHARTWIDGET_H

// EquityChartWidget — equity curve chart for the Backtest Workspace.
//
// Displays two QLineSeries on a single QChart:
//   - Strategy equity curve (portfolio value over time)
//   - Benchmark buy-and-hold curve (same initial capital, normalised)
//
// Uses QDateTimeAxis on X and QValueAxis on Y (portfolio value in $).
// Hover tooltip shows date and value for the nearest series point.

#include <QWidget>
#include <QVector>
#include "Backtest/LedgerSnapshot.h"

// Qt Charts forward declarations
class QChart;
class QChartView;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;

namespace BacktestUI {

class EquityChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit EquityChartWidget(QWidget* parent = nullptr);

    // Load the strategy equity curve. Clears any previous data.
    void setStrategyCurve(const QVector<Backtest::LedgerSnapshot>& curve);

    // Load the benchmark equity curve. Pass an empty vector to hide the series.
    void setBenchmarkCurve(const QVector<Backtest::LedgerSnapshot>& curve,
                           const QString& benchmarkSymbol = QString());

    // Convenience: set both at once.
    void setData(const QVector<Backtest::LedgerSnapshot>& strategyCurve,
                 const QVector<Backtest::LedgerSnapshot>& benchmarkCurve,
                 const QString& benchmarkSymbol = QString());

    // Clear all series.
    void clear();

private:
    void setupChart();
    void updateAxes();

    QChart*        m_chart          = nullptr;
    QChartView*    m_chartView      = nullptr;
    QLineSeries*   m_strategySeries = nullptr;
    QLineSeries*   m_benchmarkSeries = nullptr;
    QDateTimeAxis* m_axisX          = nullptr;
    QValueAxis*    m_axisY          = nullptr;
};

} // namespace BacktestUI

#endif // BACKTESTUI_EQUITYCHARTWIDGET_H
