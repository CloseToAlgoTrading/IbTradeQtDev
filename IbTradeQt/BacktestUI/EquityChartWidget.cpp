#include "BacktestUI/EquityChartWidget.h"
#include "Backtest/EquityCurvePnl.h"
#include "ThemePalette.h"
#include <QChart>
#include <QChartView>
#include <QCursor>
#include <QDateTime>
#include <QTimeZone>
#include <QDateTimeAxis>
#include <QEvent>
#include <QLineSeries>
#include <QLegendMarker>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <limits>

namespace BacktestUI {

namespace {

QVector<QPointF> snapshotsToPoints(const QVector<Backtest::LedgerSnapshot>& curve)
{
    QVector<QPointF> pts;
    pts.reserve(curve.size());
    for (const auto& s : curve)
        pts.append(QPointF(static_cast<double>(s.timestamp.toMSecsSinceEpoch()), s.portfolioValue));
    return pts;
}

} // namespace

EquityChartWidget::EquityChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setupChart();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartView);
}

void EquityChartWidget::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle(QStringLiteral("Cumulative P&L"));
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_strategySeries = new QLineSeries();
    m_strategySeries->setName(QStringLiteral("Strategy"));
    m_chart->addSeries(m_strategySeries);

    m_benchmarkSeries = new QLineSeries();
    m_benchmarkSeries->setName(QStringLiteral("Benchmark"));
    QPen benchPen = m_benchmarkSeries->pen();
    benchPen.setStyle(Qt::DashLine);
    m_benchmarkSeries->setPen(benchPen);
    m_chart->addSeries(m_benchmarkSeries);

    m_crosshairSeries = new QLineSeries();
    m_crosshairSeries->setName(QStringLiteral(""));
    QPen crossPen(QColor(180, 180, 180));
    crossPen.setWidth(1);
    crossPen.setStyle(Qt::DashLine);
    m_crosshairSeries->setPen(crossPen);
    m_chart->addSeries(m_crosshairSeries);
    for (QLegendMarker* marker : m_chart->legend()->markers(m_crosshairSeries))
        marker->setVisible(false);

    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat(QStringLiteral("MMM yyyy"));
    m_axisX->setTitleText(QStringLiteral("Date"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_strategySeries->attachAxis(m_axisX);
    m_benchmarkSeries->attachAxis(m_axisX);
    m_crosshairSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText(QStringLiteral("P&L ($)"));
    m_axisY->setLabelFormat(QStringLiteral("$%.0f"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_strategySeries->attachAxis(m_axisY);
    m_benchmarkSeries->attachAxis(m_axisY);
    m_crosshairSeries->attachAxis(m_axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMouseTracking(true);
    // Mouse move/leave are delivered to the viewport, not the QChartView itself.
    QWidget* vp = m_chartView->viewport();
    vp->setMouseTracking(true);
    vp->installEventFilter(this);
    m_chartView->setMinimumHeight(UiTheme::kBacktestChartViewMinHeight);
}

double EquityChartWidget::interpolateYAtX(const QVector<QPointF>& sortedPoints, double x)
{
    if (sortedPoints.isEmpty())
        return std::numeric_limits<double>::quiet_NaN();
    if (x <= sortedPoints.first().x())
        return sortedPoints.first().y();
    if (x >= sortedPoints.last().x())
        return sortedPoints.last().y();
    auto it = std::lower_bound(sortedPoints.begin(), sortedPoints.end(), x,
                               [](const QPointF& p, double vx) { return p.x() < vx; });
    if (it == sortedPoints.begin())
        return it->y();
    const QPointF& b = *it;
    const QPointF& a = *(it - 1);
    const double t = (x - a.x()) / (b.x() - a.x());
    return a.y() + t * (b.y() - a.y());
}

void EquityChartWidget::updateCrosshairFromScenePos(const QPointF& scenePos)
{
    m_crosshairSeries->clear();
    if (m_strategyPnlPoints.isEmpty())
        return;

    const QPointF chartPos = m_chart->mapFromScene(scenePos);
    const QPointF val      = m_chart->mapToValue(chartPos, m_strategySeries);
    if (!std::isfinite(val.x()))
        return;

    double xMs = val.x();
    const double xMin = m_strategyPnlPoints.first().x();
    const double xMax = m_strategyPnlPoints.last().x();
    xMs = qBound(xMin, xMs, xMax);

    const double ymin = m_axisY->min();
    const double ymax = m_axisY->max();
    m_crosshairSeries->append(xMs, ymin);
    m_crosshairSeries->append(xMs, ymax);

    const QDateTime dt =
        QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(xMs), QTimeZone::UTC);
    const double stratY = interpolateYAtX(m_strategyPnlPoints, xMs);
    QString tip = QStringLiteral("Time (UTC)\n%1\n\nStrategy P&L\n$%2")
                      .arg(dt.toString(Qt::ISODate))
                      .arg(stratY, 0, 'f', 2);

    if (!m_benchmarkPnlPoints.isEmpty() && m_benchmarkSeries->isVisible()) {
        const double benchY = interpolateYAtX(m_benchmarkPnlPoints, xMs);
        const QString label =
            m_benchmarkLabelForTooltip.isEmpty() ? QStringLiteral("Benchmark") : m_benchmarkLabelForTooltip;
        tip += QStringLiteral("\n\n%1\n$%2").arg(label).arg(benchY, 0, 'f', 2);
    }

    QToolTip::showText(QCursor::pos(), tip, m_chartView);
}

bool EquityChartWidget::eventFilter(QObject* watched, QEvent* event)
{
    QWidget* vp = m_chartView ? m_chartView->viewport() : nullptr;
    if (watched != vp)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::Leave) {
        m_crosshairSeries->clear();
        QToolTip::hideText();
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPointF scenePos = m_chartView->mapToScene(me->pos());
        updateCrosshairFromScenePos(scenePos);
        return QWidget::eventFilter(watched, event);
    }

    return QWidget::eventFilter(watched, event);
}

void EquityChartWidget::setStrategyCurve(const QVector<Backtest::LedgerSnapshot>& curve)
{
    m_strategySeries->clear();
    for (const auto& s : curve) {
        m_strategySeries->append(s.timestamp.toMSecsSinceEpoch(),
                                 s.portfolioValue);
    }
    updateAxes();
}

void EquityChartWidget::setBenchmarkCurve(const QVector<Backtest::LedgerSnapshot>& curve,
                                          const QString& benchmarkSymbol)
{
    m_benchmarkSeries->clear();
    m_benchmarkSeries->setVisible(!curve.isEmpty());

    if (!curve.isEmpty()) {
        const QString label = benchmarkSymbol.isEmpty()
            ? QStringLiteral("Benchmark")
            : benchmarkSymbol;
        m_benchmarkSeries->setName(label);

        for (const auto& s : curve) {
            m_benchmarkSeries->append(s.timestamp.toMSecsSinceEpoch(),
                                        s.portfolioValue);
        }
    }
    updateAxes();
}

void EquityChartWidget::setData(const QVector<Backtest::LedgerSnapshot>& strategyCurve,
                                const QVector<Backtest::LedgerSnapshot>& benchmarkCurve,
                                const QString& benchmarkSymbol,
                                double initialCapital)
{
    const QVector<Backtest::LedgerSnapshot> stratPnl =
        Backtest::equityCurveToPnlSeries(strategyCurve, initialCapital);
    const QVector<Backtest::LedgerSnapshot> benchPnl =
        Backtest::equityCurveToPnlSeries(benchmarkCurve, initialCapital);

    m_strategyPnlPoints = snapshotsToPoints(stratPnl);
    m_benchmarkPnlPoints = snapshotsToPoints(benchPnl);

    if (!benchmarkSymbol.isEmpty() || !benchmarkCurve.isEmpty()) {
        m_benchmarkLabelForTooltip =
            benchmarkSymbol.isEmpty()
                ? QStringLiteral("Benchmark (buy-and-hold)")
                : QStringLiteral("%1 (buy-and-hold)").arg(benchmarkSymbol);
    } else {
        m_benchmarkLabelForTooltip.clear();
    }

    setStrategyCurve(stratPnl);
    if (!benchmarkSymbol.isEmpty() || !benchmarkCurve.isEmpty()) {
        const QString benchLabel = benchmarkSymbol.isEmpty()
            ? QStringLiteral("Benchmark (buy-and-hold)")
            : QStringLiteral("%1 (buy-and-hold)").arg(benchmarkSymbol);
        setBenchmarkCurve(benchPnl, benchLabel);
    } else {
        setBenchmarkCurve({}, QString());
    }
}

void EquityChartWidget::clear()
{
    m_crosshairSeries->clear();
    m_strategySeries->clear();
    m_benchmarkSeries->clear();
    m_strategyPnlPoints.clear();
    m_benchmarkPnlPoints.clear();
    m_benchmarkLabelForTooltip.clear();
    m_chart->setTitle(QStringLiteral("Cumulative P&L"));
}

void EquityChartWidget::updateAxes()
{
    double minVal = std::numeric_limits<double>::max();
    double maxVal = -std::numeric_limits<double>::max();
    qint64 minTs  = std::numeric_limits<qint64>::max();
    qint64 maxTs  = -std::numeric_limits<qint64>::max();

    auto process = [&](QLineSeries* series) {
        for (const auto& pt : series->points()) {
            minVal = qMin(minVal, pt.y());
            maxVal = qMax(maxVal, pt.y());
            minTs  = qMin(minTs, static_cast<qint64>(pt.x()));
            maxTs  = qMax(maxTs, static_cast<qint64>(pt.x()));
        }
    };
    process(m_strategySeries);
    if (m_benchmarkSeries->isVisible())
        process(m_benchmarkSeries);

    if (minTs >= maxTs)
        return;

    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(minTs),
                      QDateTime::fromMSecsSinceEpoch(maxTs));

    const double span = maxVal - minVal;
    const double margin = span > 1e-12 ? span * 0.05 : 1.0;
    m_axisY->setRange(minVal - margin, maxVal + margin);
}

} // namespace BacktestUI
