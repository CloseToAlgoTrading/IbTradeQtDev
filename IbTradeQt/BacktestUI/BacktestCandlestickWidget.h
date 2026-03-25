#ifndef BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H
#define BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H

// BacktestCandlestickWidget — OHLC candlestick chart with trade overlays.
//
// X axis is QBarCategoryAxis: one category per loaded bar (label = bar date/time UTC),
// evenly spaced — no empty space for missing dates (weekends/holidays) when there is no row.
//
// Displays:
//   - QCandlestickSeries with OHLC bars from HistoricalBars cache
//   - QScatterSeries overlay for buy fills  (triangle + outline)
//   - QScatterSeries overlay for sell fills (star + outline)

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QString>
#include <QTimer>
#include "DB/dbdatatypes.h"

class QComboBox;
class QChart;
class QCandlestickSeries;
class QScatterSeries;
class QBarCategoryAxis;
class QValueAxis;
class TradingChartView;

namespace BacktestUI {

class BacktestCandlestickWidget : public QWidget {
    Q_OBJECT

public:
    explicit BacktestCandlestickWidget(QWidget* parent = nullptr);

    void setData(const QMap<QString, QList<DbHistoricalBar>>& bars,
                 const QList<DbBacktestTrade>& fills);

    void clear();

private slots:
    void onSymbolChanged(int index);
    void onChartViewRangeChanged();
    void onXLabelDebounceTimeout();

private:
    void buildChartForSymbol(const QString& symbol);
    void setupChart();
    void refitYToVisibleCandles();
    void refreshXAxisLabels();
    int  findBarIndexForEpochMs(qint64 epochMs) const;

    QComboBox*           m_symbolCombo  = nullptr;
    QChart*              m_chart        = nullptr;
    TradingChartView*    m_chartView    = nullptr;
    QCandlestickSeries*  m_candleSeries = nullptr;
    QScatterSeries*      m_buySeries    = nullptr;
    QScatterSeries*      m_sellSeries   = nullptr;
    QBarCategoryAxis*    m_axisX        = nullptr;
    QValueAxis*          m_axisY        = nullptr;

    /** Parallel to candlestick category index: bar open time (ms) for tooltips / fills. */
    QVector<qint64>      m_barEpochMs;
    /** Parallel: bar resolution string (Day1, Min1, …) for day vs intraday label formatting. */
    QVector<QString>     m_barResolutions;
    /** Precomputed unique hidden category strings (no full rebuild per zoom). */
    QVector<QString>     m_hiddenCatKeys;

    QTimer                 m_xLabelDebounceTimer;

    QMap<QString, QList<DbHistoricalBar>> m_bars;
    QList<DbBacktestTrade>                m_fills;
};

} // namespace BacktestUI

#endif // BACKTESTUI_BACKTESTCANDLESTICKWIDGET_H
