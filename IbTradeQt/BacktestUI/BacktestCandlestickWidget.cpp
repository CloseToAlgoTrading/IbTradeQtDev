#include "BacktestUI/BacktestCandlestickWidget.h"
#include <QChart>
#include <QChartView>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QScatterSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QToolTip>
#include <QPainter>
#include <limits>

namespace BacktestUI {

BacktestCandlestickWidget::BacktestCandlestickWidget(QWidget* parent)
    : QWidget(parent)
{
    setupChart();

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(QStringLiteral("Symbol:")));
    m_symbolCombo = new QComboBox();
    topRow->addWidget(m_symbolCombo);
    topRow->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(topRow);
    layout->addWidget(m_chartView);

    connect(m_symbolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BacktestCandlestickWidget::onSymbolChanged);
}

void BacktestCandlestickWidget::setupChart() {
    m_chart = new QChart();
    m_chart->setTitle(QStringLiteral("Price Chart"));
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_candleSeries = new QCandlestickSeries();
    m_candleSeries->setName(QStringLiteral("OHLC"));
    m_candleSeries->setIncreasingColor(QColor(0x26, 0xa6, 0x9a)); // teal
    m_candleSeries->setDecreasingColor(QColor(0xef, 0x53, 0x50)); // red
    m_chart->addSeries(m_candleSeries);

    m_buySeries = new QScatterSeries();
    m_buySeries->setName(QStringLiteral("Buy"));
    m_buySeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    m_buySeries->setMarkerSize(10);
    m_buySeries->setColor(QColor(0x00, 0xc8, 0x53));     // green
    m_buySeries->setBorderColor(Qt::transparent);
    m_chart->addSeries(m_buySeries);

    m_sellSeries = new QScatterSeries();
    m_sellSeries->setName(QStringLiteral("Sell"));
    m_sellSeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    m_sellSeries->setMarkerSize(10);
    m_sellSeries->setColor(QColor(0xff, 0x17, 0x44));    // red
    m_sellSeries->setBorderColor(Qt::transparent);
    m_chart->addSeries(m_sellSeries);

    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat(QStringLiteral("MMM yyyy"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_candleSeries->attachAxis(m_axisX);
    m_buySeries->attachAxis(m_axisX);
    m_sellSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText(QStringLiteral("Price ($)"));
    m_axisY->setLabelFormat(QStringLiteral("$%.2f"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_candleSeries->attachAxis(m_axisY);
    m_buySeries->attachAxis(m_axisY);
    m_sellSeries->attachAxis(m_axisY);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(300);

    connect(m_buySeries, &QScatterSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
            QToolTip::showText(QCursor::pos(),
                QString("BUY @ $%1\n%2")
                    .arg(point.y(), 0, 'f', 2)
                    .arg(dt.toString("dd MMM yyyy")), this);
        }
    });

    connect(m_sellSeries, &QScatterSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
            QToolTip::showText(QCursor::pos(),
                QString("SELL @ $%1\n%2")
                    .arg(point.y(), 0, 'f', 2)
                    .arg(dt.toString("dd MMM yyyy")), this);
        }
    });
}

void BacktestCandlestickWidget::setData(const QMap<QString, QList<DbHistoricalBar>>& bars,
                                         const QList<DbBacktestTrade>& fills) {
    m_bars  = bars;
    m_fills = fills;

    m_symbolCombo->blockSignals(true);
    m_symbolCombo->clear();
    for (const QString& sym : bars.keys())
        m_symbolCombo->addItem(sym);
    m_symbolCombo->blockSignals(false);

    if (!bars.isEmpty())
        buildChartForSymbol(bars.keys().first());
}

void BacktestCandlestickWidget::clear() {
    m_bars.clear();
    m_fills.clear();
    m_symbolCombo->clear();
    m_candleSeries->clear();
    m_buySeries->clear();
    m_sellSeries->clear();
    m_chart->setTitle(QStringLiteral("Price Chart"));
}

void BacktestCandlestickWidget::onSymbolChanged(int index) {
    Q_UNUSED(index)
    const QString sym = m_symbolCombo->currentText();
    if (!sym.isEmpty())
        buildChartForSymbol(sym);
}

void BacktestCandlestickWidget::buildChartForSymbol(const QString& symbol) {
    m_candleSeries->clear();
    m_buySeries->clear();
    m_sellSeries->clear();
    m_chart->setTitle(symbol);

    const QList<DbHistoricalBar>& bars = m_bars.value(symbol);

    double minPrice =  std::numeric_limits<double>::max();
    double maxPrice = -std::numeric_limits<double>::max();
    qint64 minTs    =  std::numeric_limits<qint64>::max();
    qint64 maxTs    = -std::numeric_limits<qint64>::max();

    for (const DbHistoricalBar& b : bars) {
        QDateTime ts = QDateTime::fromString(b.timestamp, Qt::ISODate);
        if (!ts.isValid()) continue;
        qint64 msec = ts.toMSecsSinceEpoch();

        auto* set = new QCandlestickSet(b.open, b.high, b.low, b.close, msec);
        m_candleSeries->append(set);

        minPrice = qMin(minPrice, b.low);
        maxPrice = qMax(maxPrice, b.high);
        minTs    = qMin(minTs, msec);
        maxTs    = qMax(maxTs, msec);
    }

    // Add trade fill markers for this symbol
    for (const DbBacktestTrade& t : m_fills) {
        if (t.symbol != symbol) continue;
        QDateTime ts = QDateTime::fromString(t.timestamp, Qt::ISODate);
        if (!ts.isValid()) continue;
        qint64 msec = ts.toMSecsSinceEpoch();

        if (t.side == QLatin1String("BUY"))
            m_buySeries->append(msec, t.fillPrice);
        else
            m_sellSeries->append(msec, t.fillPrice);

        minPrice = qMin(minPrice, t.fillPrice);
        maxPrice = qMax(maxPrice, t.fillPrice);
    }

    if (minTs < maxTs) {
        m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(minTs),
                           QDateTime::fromMSecsSinceEpoch(maxTs));
        const double margin = (maxPrice - minPrice) * 0.05;
        m_axisY->setRange(minPrice - margin, maxPrice + margin);
    }
}

} // namespace BacktestUI
