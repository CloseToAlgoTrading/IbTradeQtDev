#include "SharedUI/TradingChartView.h"

#include <QBarCategoryAxis>
#include <QChart>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QMouseEvent>
#include <QValueAxis>
#include <QWheelEvent>
#include <QPainter>
#include <QtMath>
#include <cmath>

namespace {

constexpr qint64 kMinTimeSpanMs = 1000;
constexpr double kMinValueSpan  = 1e-9;
constexpr int    kMinCategorySpan = 1;

double zoomScaleFromWheel(qreal angleDeltaY)
{
    const qreal step = angleDeltaY / 120.0;
    return std::pow(0.9, step);
}

} // namespace

TradingChartView::TradingChartView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
    , m_chart(chart)
{
    setRubberBand(QChartView::RectangleRubberBand);
    setRenderHint(QPainter::Antialiasing);
    setMouseTracking(true);
    if (viewport())
        viewport()->setMouseTracking(true);
}

void TradingChartView::setTimeAxis(QDateTimeAxis* axis)
{
    m_axisXDateTime  = axis;
    m_axisXCategory  = nullptr;
    m_xIsCategory    = false;
}

void TradingChartView::setCategoryAxis(QBarCategoryAxis* axis)
{
    m_axisXCategory = axis;
    m_axisXDateTime = nullptr;
    m_xIsCategory   = true;
}

void TradingChartView::setValueAxis(QValueAxis* axis)
{
    m_axisY = axis;
}

void TradingChartView::setMappingSeries(QAbstractSeries* series)
{
    m_mappingSeries = series;
}

QAbstractSeries* TradingChartView::mappingSeries() const
{
    return m_mappingSeries;
}

void TradingChartView::setFullRange(qint64 xMinMs, qint64 xMaxMs, double yMin, double yMax)
{
    m_hasFullRange = true;
    m_xIsCategory  = false;
    m_fullXMinMs   = xMinMs;
    m_fullXMaxMs   = xMaxMs;
    m_fullYMin     = yMin;
    m_fullYMax     = yMax;
}

void TradingChartView::setFullCategoryRange(int minIdx, int maxIdx, double yMin, double yMax)
{
    m_hasFullRange = true;
    m_xIsCategory  = true;
    m_fullIdxMin   = qMin(minIdx, maxIdx);
    m_fullIdxMax   = qMax(minIdx, maxIdx);
    m_fullYMin     = yMin;
    m_fullYMax     = yMax;
}

void TradingChartView::clearFullRange()
{
    m_hasFullRange = false;
}

bool TradingChartView::visibleCategoryBarRange(int* outMinIdx, int* outMaxIdx) const
{
    return categoryVisibleRange(outMinIdx, outMaxIdx);
}

QAbstractSeries* TradingChartView::effectiveMappingSeries() const
{
    if (m_mappingSeries)
        return m_mappingSeries;
    if (!m_chart || m_chart->series().isEmpty())
        return nullptr;
    return m_chart->series().first();
}

bool TradingChartView::categoryVisibleRange(int* outMinIdx, int* outMaxIdx) const
{
    if (!m_axisXCategory)
        return false;
    const QStringList cats = m_axisXCategory->categories();
    QString kmin = m_axisXCategory->min();
    QString kmax = m_axisXCategory->max();
    if (kmin.isEmpty() || kmax.isEmpty() || cats.isEmpty())
        return false;
    int i0 = cats.indexOf(kmin);
    int i1 = cats.indexOf(kmax);
    if (i0 < 0 || i1 < 0)
        return false;
    *outMinIdx = qMin(i0, i1);
    *outMaxIdx = qMax(i0, i1);
    return true;
}

void TradingChartView::clampXDateTime()
{
    if (!m_axisXDateTime)
        return;

    qint64 xMin = m_axisXDateTime->min().toMSecsSinceEpoch();
    qint64 xMax = m_axisXDateTime->max().toMSecsSinceEpoch();

    const qint64 spanFullX = m_fullXMaxMs - m_fullXMinMs;
    if (spanFullX <= 0) {
        const qint64 pad = qMax(qint64(1), kMinTimeSpanMs / 2);
        m_axisXDateTime->setRange(QDateTime::fromMSecsSinceEpoch(m_fullXMinMs - pad),
                                  QDateTime::fromMSecsSinceEpoch(m_fullXMaxMs + pad));
    } else {
        qint64 xSpan = xMax - xMin;
        xSpan          = qBound(kMinTimeSpanMs, xSpan, spanFullX);
        const qint64 hi = m_fullXMaxMs - xSpan;
        if (hi >= m_fullXMinMs)
            xMin = qBound(m_fullXMinMs, xMin, hi);
        else
            xMin = m_fullXMinMs;
        xMax = xMin + xSpan;
        m_axisXDateTime->setRange(QDateTime::fromMSecsSinceEpoch(xMin),
                                  QDateTime::fromMSecsSinceEpoch(xMax));
    }
}

void TradingChartView::clampXCategory()
{
    if (!m_axisXCategory || !m_axisY)
        return;

    const QStringList cats = m_axisXCategory->categories();
    if (cats.isEmpty())
        return;

    int iMin = 0;
    int iMax = 0;
    if (!categoryVisibleRange(&iMin, &iMax))
        return;

    const int spanFull = m_fullIdxMax - m_fullIdxMin + 1;
    int       span     = iMax - iMin + 1;
    if (spanFull > 0)
        span = qBound(kMinCategorySpan, span, spanFull);
    else
        span = kMinCategorySpan;

    const int hi = m_fullIdxMax - span + 1;
    if (hi >= m_fullIdxMin)
        iMin = qBound(m_fullIdxMin, iMin, hi);
    else
        iMin = m_fullIdxMin;
    iMax = iMin + span - 1;

    m_axisXCategory->setRange(cats.at(iMin), cats.at(iMax));
}

void TradingChartView::clampY()
{
    if (!m_axisY)
        return;

    double yMin = m_axisY->min();
    double yMax = m_axisY->max();

    const double spanFullY = m_fullYMax - m_fullYMin;
    if (spanFullY <= 0.0) {
        const double pad = qMax(1e-6, kMinValueSpan);
        m_axisY->setRange(m_fullYMin - pad, m_fullYMax + pad);
        return;
    }

    double ySpan = yMax - yMin;
    ySpan          = qBound(kMinValueSpan, ySpan, spanFullY);
    const double hiY = m_fullYMax - ySpan;
    if (hiY >= m_fullYMin)
        yMin = qBound(m_fullYMin, yMin, hiY);
    else
        yMin = m_fullYMin;
    yMax = yMin + ySpan;
    m_axisY->setRange(yMin, yMax);
}

void TradingChartView::clampToFullRange()
{
    if (!m_hasFullRange || !m_axisY)
        return;

    if (m_xIsCategory && m_axisXCategory) {
        clampXCategory();
        clampY();
        return;
    }
    if (m_axisXDateTime) {
        clampXDateTime();
        clampY();
    }
}

void TradingChartView::applyWheelZoom(const QPointF& widgetPos, double scale)
{
    if (m_xIsCategory && m_axisXCategory) {
        applyWheelZoomCategory(widgetPos, scale);
        return;
    }
    QAbstractSeries* s = effectiveMappingSeries();
    if (!m_axisXDateTime || !m_axisY || !m_chart || !s)
        return;

    const QPointF scenePos = mapToScene(widgetPos.toPoint());
    const QPointF chartPos = m_chart->mapFromScene(scenePos);
    const QPointF val      = m_chart->mapToValue(chartPos, s);
    if (!std::isfinite(val.x()) || !std::isfinite(val.y()))
        return;

    const qint64 xMin = m_axisXDateTime->min().toMSecsSinceEpoch();
    const qint64 xMax = m_axisXDateTime->max().toMSecsSinceEpoch();
    const double yMin = m_axisY->min();
    const double yMax = m_axisY->max();

    qint64 xSpan = xMax - xMin;
    double ySpan = yMax - yMin;
    if (xSpan <= 0 || ySpan <= 0.0)
        return;

    const qint64 xMs = static_cast<qint64>(val.x());
    const double y   = val.y();

    const double relX = double(xMs - xMin) / double(xSpan);
    const double relY = (y - yMin) / ySpan;

    qint64 newSpanX = static_cast<qint64>(double(xSpan) * scale);
    double newSpanY = ySpan * scale;
    if (newSpanX < kMinTimeSpanMs)
        newSpanX = kMinTimeSpanMs;
    if (newSpanY < kMinValueSpan)
        newSpanY = kMinValueSpan;

    qint64 newXMin = xMs - static_cast<qint64>(relX * double(newSpanX));
    qint64 newXMax = newXMin + newSpanX;
    double newYMin = y - relY * newSpanY;
    double newYMax = newYMin + newSpanY;

    m_axisXDateTime->setRange(QDateTime::fromMSecsSinceEpoch(newXMin),
                              QDateTime::fromMSecsSinceEpoch(newXMax));
    m_axisY->setRange(newYMin, newYMax);
    clampToFullRange();
}

void TradingChartView::applyWheelZoomCategory(const QPointF& widgetPos, double scale)
{
    QAbstractSeries* s = effectiveMappingSeries();
    if (!m_axisXCategory || !m_axisY || !m_chart || !s)
        return;

    const QStringList cats = m_axisXCategory->categories();
    if (cats.isEmpty())
        return;

    const QPointF scenePos = mapToScene(widgetPos.toPoint());
    const QPointF chartPos = m_chart->mapFromScene(scenePos);
    const QPointF val      = m_chart->mapToValue(chartPos, s);
    if (!std::isfinite(val.x()) || !std::isfinite(val.y()))
        return;

    int iMin = 0;
    int iMax = 0;
    if (!categoryVisibleRange(&iMin, &iMax))
        return;

    const int nVis = iMax - iMin + 1;
    if (nVis < 1)
        return;

    const double x    = val.x();
    const double relX = nVis > 1 ? (x - double(iMin)) / double(nVis - 1) : 0.5;

    int newNV = qMax(1, qRound(double(nVis) * scale));
    newNV     = qMin(newNV, m_fullIdxMax - m_fullIdxMin + 1);

    int newMin = static_cast<int>(qRound(x - relX * double(newNV - 1)));
    int newMax = newMin + newNV - 1;
    newMin     = qBound(m_fullIdxMin, newMin, m_fullIdxMax);
    newMax     = qBound(m_fullIdxMin, newMax, m_fullIdxMax);
    if (newMax - newMin + 1 < newNV) {
        newMin = qMax(m_fullIdxMin, newMax - newNV + 1);
    }
    m_axisXCategory->setRange(cats.at(newMin), cats.at(newMax));
    clampToFullRange();
}

void TradingChartView::applyPanDelta(const QPoint& delta)
{
    if (m_xIsCategory && m_axisXCategory) {
        applyPanDeltaCategory(delta);
        return;
    }
    if (!m_axisXDateTime || !m_axisY || !m_chart)
        return;

    const QRectF plotRect = m_chart->plotArea();
    const qreal  plotW    = plotRect.width();
    const qreal  plotH    = plotRect.height();
    if (plotW <= 1e-6 || plotH <= 1e-6)
        return;

    const qint64 xMin = m_axisXDateTime->min().toMSecsSinceEpoch();
    const qint64 xMax = m_axisXDateTime->max().toMSecsSinceEpoch();
    const double yMin = m_axisY->min();
    const double yMax = m_axisY->max();

    const qint64 xSpan = xMax - xMin;
    const double ySpan = yMax - yMin;

    const double dxFrac = double(delta.x()) / double(plotW);
    const double dyFrac = -double(delta.y()) / double(plotH);

    const qint64 shiftX = static_cast<qint64>(dxFrac * double(xSpan));
    const double shiftY = dyFrac * ySpan;

    qint64 newXMin = xMin - shiftX;
    qint64 newXMax = xMax - shiftX;
    double newYMin = yMin + shiftY;
    double newYMax = yMax + shiftY;

    m_axisXDateTime->setRange(QDateTime::fromMSecsSinceEpoch(newXMin),
                              QDateTime::fromMSecsSinceEpoch(newXMax));
    m_axisY->setRange(newYMin, newYMax);
    clampToFullRange();
}

void TradingChartView::applyPanDeltaCategory(const QPoint& delta)
{
    if (!m_axisXCategory || !m_axisY || !m_chart)
        return;

    const QStringList cats = m_axisXCategory->categories();
    if (cats.isEmpty())
        return;

    const QRectF plotRect = m_chart->plotArea();
    const qreal  plotW    = plotRect.width();
    const qreal  plotH    = plotRect.height();
    if (plotW <= 1e-6 || plotH <= 1e-6)
        return;

    int iMin = 0;
    int iMax = 0;
    if (!categoryVisibleRange(&iMin, &iMax))
        return;

    const int nVis = iMax - iMin + 1;
    const double dxFrac = double(delta.x()) / double(plotW);
    const double dyFrac = -double(delta.y()) / double(plotH);

    const int shiftIdx = static_cast<int>(qRound(dxFrac * double(nVis)));
    const double ySpan = m_axisY->max() - m_axisY->min();
    const double shiftY = dyFrac * ySpan;

    const int span = iMax - iMin;
    int newMin = iMin - shiftIdx;
    newMin     = qBound(m_fullIdxMin, newMin, m_fullIdxMax - span);
    const int newMax = newMin + span;

    m_axisXCategory->setRange(cats.at(newMin), cats.at(newMax));

    const double yMin = m_axisY->min() + shiftY;
    const double yMax = m_axisY->max() + shiftY;
    m_axisY->setRange(yMin, yMax);
    clampToFullRange();
}

void TradingChartView::resetToDataRange()
{
    if (!m_axisY || !m_hasFullRange)
        return;
    if (m_xIsCategory && m_axisXCategory) {
        const QStringList cats = m_axisXCategory->categories();
        if (cats.isEmpty() || m_fullIdxMin < 0 || m_fullIdxMax >= cats.size())
            return;
        m_axisXCategory->setRange(cats.at(m_fullIdxMin), cats.at(m_fullIdxMax));
        m_axisY->setRange(m_fullYMin, m_fullYMax);
        emit viewRangeChanged();
        return;
    }
    if (m_axisXDateTime) {
        m_axisXDateTime->setRange(QDateTime::fromMSecsSinceEpoch(m_fullXMinMs),
                                  QDateTime::fromMSecsSinceEpoch(m_fullXMaxMs));
        m_axisY->setRange(m_fullYMin, m_fullYMax);
        emit viewRangeChanged();
    }
}

void TradingChartView::wheelEvent(QWheelEvent* event)
{
    const bool haveX = (m_xIsCategory && m_axisXCategory) || m_axisXDateTime;
    if (!haveX || !m_axisY || !effectiveMappingSeries()) {
        QChartView::wheelEvent(event);
        return;
    }
    const double scale = zoomScaleFromWheel(event->angleDelta().y());
    if (qFuzzyCompare(scale, 1.0)) {
        QChartView::wheelEvent(event);
        return;
    }
    applyWheelZoom(event->position(), scale);
    event->accept();
    emit viewRangeChanged();
}

void TradingChartView::mousePressEvent(QMouseEvent* event)
{
    const bool pan = event->button() == Qt::MiddleButton
                     || (event->button() == Qt::LeftButton
                         && (event->modifiers() & Qt::AltModifier));
    if (pan) {
        m_isPanning  = true;
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void TradingChartView::mouseMoveEvent(QMouseEvent* event)
{
    const bool haveX = (m_xIsCategory && m_axisXCategory) || m_axisXDateTime;
    if (m_isPanning && haveX && m_axisY) {
        const QPoint d = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        applyPanDelta(d);
        event->accept();
        return;
    }
    QChartView::mouseMoveEvent(event);
}

void TradingChartView::mouseReleaseEvent(QMouseEvent* event)
{
    const bool pan = event->button() == Qt::MiddleButton
                     || (event->button() == Qt::LeftButton
                         && (event->modifiers() & Qt::AltModifier));
    if (m_isPanning && pan) {
        m_isPanning = false;
        event->accept();
        emit viewRangeChanged();
        return;
    }

    QChartView::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton && !m_isPanning) {
        bool ok = false;
        if (m_axisY && m_axisY->min() < m_axisY->max()) {
            if (m_xIsCategory && m_axisXCategory) {
                int a = 0;
                int b = 0;
                ok = categoryVisibleRange(&a, &b) && a <= b;
            } else if (m_axisXDateTime) {
                ok = m_axisXDateTime->min().toMSecsSinceEpoch()
                     < m_axisXDateTime->max().toMSecsSinceEpoch();
            }
        }
        if (ok)
            emit viewRangeChanged();
    }
}

void TradingChartView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        resetToDataRange();
        event->accept();
        return;
    }
    QChartView::mouseDoubleClickEvent(event);
}
