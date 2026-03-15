#include "BacktestUI/EquityChartWidget.h"
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QToolTip>
#include <QDateTime>
#include <QtMath>

namespace BacktestUI {

EquityChartWidget::EquityChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setupChart();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartView);
}

void EquityChartWidget::setupChart() {
    m_chart = new QChart();
    m_chart->setTitle(QStringLiteral("Equity Curve"));
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

    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat(QStringLiteral("MMM yyyy"));
    m_axisX->setTitleText(QStringLiteral("Date"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_strategySeries->attachAxis(m_axisX);
    m_benchmarkSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText(QStringLiteral("Portfolio Value ($)"));
    m_axisY->setLabelFormat(QStringLiteral("$%.0f"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_strategySeries->attachAxis(m_axisY);
    m_benchmarkSeries->attachAxis(m_axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(300);

    // Hover tooltip showing date / value
    connect(m_strategySeries, &QLineSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
            QToolTip::showText(QCursor::pos(),
                QString("Strategy\n%1\n$%2")
                    .arg(dt.toString("dd MMM yyyy"))
                    .arg(point.y(), 0, 'f', 2),
                this);
        }
    });

    connect(m_benchmarkSeries, &QLineSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
            QToolTip::showText(QCursor::pos(),
                QString("Benchmark\n%1\n$%2")
                    .arg(dt.toString("dd MMM yyyy"))
                    .arg(point.y(), 0, 'f', 2),
                this);
        }
    });
}

void EquityChartWidget::setStrategyCurve(const QVector<Backtest::LedgerSnapshot>& curve) {
    m_strategySeries->clear();
    for (const auto& s : curve) {
        m_strategySeries->append(s.timestamp.toMSecsSinceEpoch(),
                                  s.portfolioValue);
    }
    updateAxes();
}

void EquityChartWidget::setBenchmarkCurve(const QVector<Backtest::LedgerSnapshot>& curve,
                                           const QString& benchmarkSymbol) {
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
                                 const QString& benchmarkSymbol) {
    setStrategyCurve(strategyCurve);
    setBenchmarkCurve(benchmarkCurve, benchmarkSymbol);
}

void EquityChartWidget::clear() {
    m_strategySeries->clear();
    m_benchmarkSeries->clear();
    m_chart->setTitle(QStringLiteral("Equity Curve"));
}

void EquityChartWidget::updateAxes() {
    // Compute combined min/max for both series
    double minVal =  std::numeric_limits<double>::max();
    double maxVal = -std::numeric_limits<double>::max();
    qint64 minTs  =  std::numeric_limits<qint64>::max();
    qint64 maxTs  = -std::numeric_limits<qint64>::max();

    auto process = [&](QLineSeries* series) {
        for (const auto& pt : series->points()) {
            minVal = qMin(minVal, pt.y());
            maxVal = qMax(maxVal, pt.y());
            minTs  = qMin(minTs,  static_cast<qint64>(pt.x()));
            maxTs  = qMax(maxTs,  static_cast<qint64>(pt.x()));
        }
    };
    process(m_strategySeries);
    if (m_benchmarkSeries->isVisible()) process(m_benchmarkSeries);

    if (minTs >= maxTs) return;

    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(minTs),
                       QDateTime::fromMSecsSinceEpoch(maxTs));

    const double margin = (maxVal - minVal) * 0.05;
    m_axisY->setRange(minVal - margin, maxVal + margin);
}

} // namespace BacktestUI
