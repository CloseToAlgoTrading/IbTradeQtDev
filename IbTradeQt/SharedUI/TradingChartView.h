#pragma once

#include <QChartView>

class QChart;
class QDateTimeAxis;
class QBarCategoryAxis;
class QValueAxis;
class QAbstractSeries;

/**
 * QChartView with wheel zoom, rubber-band zoom, pan (middle / Alt+left), double-click reset.
 *
 * Horizontal axis: either QDateTimeAxis (e.g. equity chart) or QBarCategoryAxis (ordinal bars,
 * e.g. candlesticks with no weekend gaps). Call exactly one of setTimeAxis / setCategoryAxis.
 */
class TradingChartView : public QChartView {
    Q_OBJECT

public:
    explicit TradingChartView(QChart* chart, QWidget* parent = nullptr);

    void setTimeAxis(QDateTimeAxis* axis);
    void setCategoryAxis(QBarCategoryAxis* axis);
    void setValueAxis(QValueAxis* axis);

    /** Series used for mapToValue during zoom (defaults to chart's first series). */
    void setMappingSeries(QAbstractSeries* series);
    QAbstractSeries* mappingSeries() const;

    /** Full extents for QDateTimeAxis (ms) + Y. */
    void setFullRange(qint64 xMinMs, qint64 xMaxMs, double yMin, double yMax);

    /** Full extents for QBarCategoryAxis: inclusive bar indices matching category strings. */
    void setFullCategoryRange(int minIdx, int maxIdx, double yMin, double yMax);

    void clearFullRange();

    void resetToDataRange();

    /** Visible inclusive bar indices for QBarCategoryAxis (false if axis/range invalid). */
    bool visibleCategoryBarRange(int* outMinIdx, int* outMaxIdx) const;

signals:
    /** Emitted after user zoom/pan/rubber-band zoom or programmatic reset. */
    void viewRangeChanged();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void applyWheelZoom(const QPointF& widgetPos, double scale);
    void applyPanDelta(const QPoint& delta);
    void clampToFullRange();
    void clampXDateTime();
    void clampXCategory();
    void clampY();
    void applyWheelZoomCategory(const QPointF& widgetPos, double scale);
    void applyPanDeltaCategory(const QPoint& delta);
    bool categoryVisibleRange(int* outMinIdx, int* outMaxIdx) const;
    QAbstractSeries* effectiveMappingSeries() const;

    QChart*          m_chart          = nullptr;
    QDateTimeAxis*   m_axisXDateTime  = nullptr;
    QBarCategoryAxis* m_axisXCategory = nullptr;
    QValueAxis*      m_axisY          = nullptr;
    QAbstractSeries* m_mappingSeries  = nullptr;

    bool   m_hasFullRange = false;
    bool   m_xIsCategory  = false;
    qint64 m_fullXMinMs   = 0;
    qint64 m_fullXMaxMs   = 0;
    int    m_fullIdxMin   = 0;
    int    m_fullIdxMax   = 0;
    double m_fullYMin     = 0.0;
    double m_fullYMax     = 0.0;

    bool   m_isPanning = false;
    QPoint m_lastPanPos;
};
