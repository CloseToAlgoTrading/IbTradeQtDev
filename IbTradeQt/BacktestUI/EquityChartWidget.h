#ifndef BACKTESTUI_EQUITYCHARTWIDGET_H
#define BACKTESTUI_EQUITYCHARTWIDGET_H

// EquityChartWidget — equity curve chart for the Backtest Workspace.
//
// Displays two QLineSeries on a single QChart:
//   - Strategy cumulative P&L (portfolio value − initial capital)
//   - Benchmark buy-and-hold cumulative P&L (same initial capital, normalised)
//
// Uses QDateTimeAxis on X and QValueAxis on Y (P&L in $).
// Hover tooltip shows date and value for the nearest series point.

#include <QVector>
#include <QWidget>
#include "Backtest/LedgerSnapshot.h"

class QEvent;
class QObject;
class QChart;
class QLineSeries;
class TradingChartView;
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

    /// Cumulative P&L curves; benchmark hidden when \a benchmarkCurve is empty.
    void setData(const QVector<Backtest::LedgerSnapshot>& strategyCurve,
                 const QVector<Backtest::LedgerSnapshot>& benchmarkCurve,
                 const QString& benchmarkSymbol,
                 double initialCapital);

    /// Same as setData but \a strategyPnl / \a benchmarkPnl are already P&L series (off-thread prep).
    void setPnlSeriesData(const QVector<Backtest::LedgerSnapshot>& strategyPnl,
                          const QVector<Backtest::LedgerSnapshot>& benchmarkPnl,
                          const QString& benchmarkSymbol,
                          bool hasBenchmarkData);

    // Clear all series.
    void clear();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onChartViewRangeChanged();

private:
    void setupChart();
    void updateAxes();
    void refitYToVisiblePnl();
    void updateCrosshairFromScenePos(const QPointF& scenePos);

    static double interpolateYAtX(const QVector<QPointF>& sortedPoints, double x);

    QChart*           m_chart            = nullptr;
    TradingChartView* m_chartView        = nullptr;
    QLineSeries*   m_strategySeries   = nullptr;
    QLineSeries*   m_benchmarkSeries  = nullptr;
    QLineSeries*   m_crosshairSeries  = nullptr;
    QDateTimeAxis* m_axisX            = nullptr;
    QValueAxis*    m_axisY            = nullptr;
    /// PnL points (x = ms since epoch) for tooltip / crosshair interpolation.
    QVector<QPointF> m_strategyPnlPoints;
    QVector<QPointF> m_benchmarkPnlPoints;
    QString          m_benchmarkLabelForTooltip;
};

} // namespace BacktestUI

#endif // BACKTESTUI_EQUITYCHARTWIDGET_H
