#ifndef BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H
#define BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H

// BacktestCandlestickWidget — OHLC candlestick chart with trade overlays.
//
// Displays:
//   - QCandlestickSeries with OHLC bars from HistoricalBars cache
//   - QScatterSeries overlay for buy fills  (green triangle pointing up)
//   - QScatterSeries overlay for sell fills (red   triangle pointing down)
//
// A symbol combo-box allows switching between symbols when multiple were
// traded. Bar data and fills are supplied after a run completes via setData().

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QString>
#include "DB/dbdatatypes.h"

class QComboBox;
class QChart;
class QChartView;
class QCandlestickSeries;
class QScatterSeries;
class QDateTimeAxis;
class QValueAxis;
class QBarCategoryAxis;

namespace BacktestUI {

class BacktestCandlestickWidget : public QWidget {
    Q_OBJECT

public:
    explicit BacktestCandlestickWidget(QWidget* parent = nullptr);

    // Supply all data. bars: symbol -> list of OHLC bars.
    // fills: list of all trade fills (filtered per symbol on selection change).
    void setData(const QMap<QString, QList<DbHistoricalBar>>& bars,
                 const QList<DbBacktestTrade>& fills);

    // Clear all data.
    void clear();

private slots:
    void onSymbolChanged(int index);

private:
    void buildChartForSymbol(const QString& symbol);
    void setupChart();

    QComboBox*          m_symbolCombo  = nullptr;
    QChart*             m_chart        = nullptr;
    QChartView*         m_chartView    = nullptr;
    QCandlestickSeries* m_candleSeries = nullptr;
    QScatterSeries*     m_buySeries    = nullptr;
    QScatterSeries*     m_sellSeries   = nullptr;
    QDateTimeAxis*      m_axisX        = nullptr;
    QValueAxis*         m_axisY        = nullptr;

    QMap<QString, QList<DbHistoricalBar>> m_bars;
    QList<DbBacktestTrade>                m_fills;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H
