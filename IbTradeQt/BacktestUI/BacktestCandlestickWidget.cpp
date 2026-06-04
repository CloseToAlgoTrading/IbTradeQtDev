#include "BacktestUI/BacktestCandlestickWidget.h"
#include "SharedUI/TradingChartTheme.h"
#include "SharedUI/TradingChartView.h"
#include "ThemePalette.h"
#include <QChart>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QScatterSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QToolTip>
#include <QSignalBlocker>
#include <QPainter>
#include <QPen>
#include <QCursor>
#include <QLocale>
#include <QHash>
#include <QTimeZone>
#include <QSet>
#include <algorithm>
#include <limits>

namespace BacktestUI {

namespace {

/** Max readable tick labels on X; QBarCategoryAxis draws one string per category slot — we only set text here. */
constexpr int kMaxXAxisLabels = 12;

/** Unique per index using only ZWSP / ZWNJ (zero width). PUA U+E000… renders as “/” with Qt Charts’ font. */
QString hiddenCatKey(int i)
{
    quint32 v = static_cast<quint32>(qMax(0, i));
    QString s;
    s.reserve(32);
    for (int b = 0; b < 32; ++b) {
        s.append(QChar((v & 1) ? 0x200B : 0x200C));
        v >>= 1;
    }
    return s;
}

/** Evenly spaced bar indices in [iMin, iMax] (inclusive), at most maxLabels entries. */
QVector<int> pickLabelIndices(int iMin, int iMax, int maxLabels)
{
    const int vb = iMax - iMin + 1;
    if (vb <= 0 || maxLabels <= 0)
        return {};
    const int nPick = qMin(maxLabels, vb);
    QVector<int> out;
    out.reserve(nPick);
    if (nPick == 1) {
        out.append(iMin);
        return out;
    }
    for (int j = 0; j < nPick; ++j)
        out.append(iMin + (j * (vb - 1)) / (nPick - 1));
    return out;
}

/** Max escalation steps when tick strings collide; last step should be unique for real bars. */
constexpr int kMaxXLabelDetailLevels = 6;

/**
 * Visible-window tick text. `detailLevel` increases precision when labels must disambiguate
 * (never use "(2)" suffixes — escalate format instead).
 */
QString formatXTickLabel(const QDateTime& utc, const QString& resolution, double spanDays, int nTicks,
                         int detailLevel)
{
    const QString r = resolution.toLower();
    const QLocale loc(QLocale::English);

    const bool isDaily = r.contains(QLatin1String("day"), Qt::CaseInsensitive);
    const bool multiYearUi = (nTicks >= 3 && spanDays >= 400.0);

    if (detailLevel >= 5)
        return utc.toString(Qt::ISODateWithMs);

    auto pick = [&](const QStringList& patterns) -> QString {
        const int i = qBound(0, detailLevel, patterns.size() - 1);
        return loc.toString(utc, patterns.at(i));
    };

    if (multiYearUi) {
        const QStringList pats = {QStringLiteral("MMM yyyy"), QStringLiteral("dd MMM yyyy"),
                                  QStringLiteral("dd MMM yyyy HH:mm"), QStringLiteral("dd MMM yyyy HH:mm:ss")};
        return pick(pats);
    }

    // Very zoomed in: time detail for sub-daily bars.
    if (spanDays <= 2.5) {
        const QStringList patsDaily
            = {QStringLiteral("dd MMM yy"), QStringLiteral("dd MMM yyyy"), QStringLiteral("dd MMM yyyy")};
        if (isDaily)
            return pick(patsDaily);
        const QStringList patsIntra = {QStringLiteral("dd MMM yy HH:mm"),
                                       QStringLiteral("dd MMM yy HH:mm:ss"),
                                       QStringLiteral("dd MMM yyyy HH:mm:ss")};
        if (r.contains(QLatin1String("sec")))
            return pick({QStringLiteral("dd MMM yy HH:mm:ss"), QStringLiteral("dd MMM yyyy HH:mm:ss")});
        return pick(patsIntra);
    }
    if (spanDays < 400.0) {
        const QStringList patsDaily
            = {QStringLiteral("dd MMM yy"), QStringLiteral("dd MMM yyyy"), QStringLiteral("dd MMM yyyy")};
        if (isDaily)
            return pick(patsDaily);
        return pick({QStringLiteral("dd MMM yy HH:mm"), QStringLiteral("dd MMM yyyy HH:mm"),
                     QStringLiteral("dd MMM yyyy HH:mm:ss")});
    }
    if (spanDays <= 900.0)
        return pick({QStringLiteral("MMM yy"), QStringLiteral("MMM yyyy"), QStringLiteral("dd MMM yyyy"),
                     QStringLiteral("dd MMM yyyy HH:mm")});
    return pick({QStringLiteral("yyyy"), QStringLiteral("MMM yyyy"), QStringLiteral("dd MMM yyyy")});
}

bool barLessByTime(const DbHistoricalBar& a, const DbHistoricalBar& b)
{
    const QDateTime ta = QDateTime::fromString(a.timestamp, Qt::ISODate);
    const QDateTime tb = QDateTime::fromString(b.timestamp, Qt::ISODate);
    if (ta != tb)
        return ta < tb;
    return a.timestamp < b.timestamp;
}

} // namespace

BacktestCandlestickWidget::BacktestCandlestickWidget(QWidget* parent)
    : QWidget(parent)
{
    setupChart();

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(QStringLiteral("Symbol:")));
    m_symbolCombo = new QComboBox();
    m_symbolCombo->setToolTip(
        QStringLiteral("Selects which symbol from the current backtest result is displayed in the candlestick chart."));
    topRow->addWidget(m_symbolCombo);
    topRow->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(topRow);
    layout->addWidget(m_chartView);

    connect(m_symbolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BacktestCandlestickWidget::onSymbolChanged);
    connect(m_chartView, &TradingChartView::viewRangeChanged,
            this, &BacktestCandlestickWidget::onChartViewRangeChanged);

    m_xLabelDebounceTimer.setSingleShot(true);
    m_xLabelDebounceTimer.setInterval(200);
    connect(&m_xLabelDebounceTimer, &QTimer::timeout,
            this, &BacktestCandlestickWidget::onXLabelDebounceTimeout);
}

void BacktestCandlestickWidget::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle(QStringLiteral("Price Chart"));
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_candleSeries = new QCandlestickSeries();
    m_candleSeries->setName(QStringLiteral("OHLC"));
    m_chart->addSeries(m_candleSeries);

    m_buySeries = new QScatterSeries();
    m_buySeries->setName(QStringLiteral("Buy (entry)"));
    m_buySeries->setMarkerShape(QScatterSeries::MarkerShapeTriangle);
    m_buySeries->setMarkerSize(18.0);
    m_buySeries->setColor(QColor(QString::fromLatin1(UiTheme::kChartBuyMarker)));
    m_buySeries->setBorderColor(QColor(QString::fromLatin1(UiTheme::kTextPrimary)));
    m_chart->addSeries(m_buySeries);

    m_sellSeries = new QScatterSeries();
    m_sellSeries->setName(QStringLiteral("Sell (exit)"));
    m_sellSeries->setMarkerShape(QScatterSeries::MarkerShapeStar);
    m_sellSeries->setMarkerSize(18.0);
    m_sellSeries->setColor(QColor(QString::fromLatin1(UiTheme::kChartSellMarker)));
    m_sellSeries->setBorderColor(QColor(QString::fromLatin1(UiTheme::kTextPrimary)));
    m_chart->addSeries(m_sellSeries);

    m_axisX = new QBarCategoryAxis();
    m_axisX->setTitleText(QStringLiteral("Date / time (UTC)"));
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

    m_chartView = new TradingChartView(m_chart);
    m_chartView->setObjectName(QStringLiteral("tradingChartView"));
    m_chartView->setCategoryAxis(m_axisX);
    m_chartView->setValueAxis(m_axisY);
    m_chartView->setMappingSeries(m_candleSeries);
    m_chartView->setMinimumHeight(UiTheme::kBacktestChartViewMinHeight);

    TradingChartTheme::applyDarkTheme(m_chart);
    m_chart->setAnimationOptions(QChart::NoAnimation);

    m_candleSeries->setIncreasingColor(TradingChartTheme::candleBullColor());
    m_candleSeries->setDecreasingColor(TradingChartTheme::candleBearColor());
    // Gap between bodies (ordinal X: no overlap when zoomed out — avoid min pixel width).
    m_candleSeries->setBodyWidth(0.55);
    m_candleSeries->setBodyOutlineVisible(true);
    // Horizontal caps on wicks read like boxplot whiskers; standard candles use plain wicks only.
    m_candleSeries->setCapsVisible(false);
    QPen candlePen(QColor(QString::fromLatin1(UiTheme::kChartCandleStroke)));
    candlePen.setWidthF(1.0);
    candlePen.setCosmetic(true);
    m_candleSeries->setPen(candlePen);

    connect(m_buySeries, &QScatterSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            const int idx = static_cast<int>(point.x());
            QString dtStr;
            if (idx >= 0 && idx < m_barEpochMs.size())
                dtStr = QDateTime::fromMSecsSinceEpoch(m_barEpochMs.at(idx)).toString(QStringLiteral("dd MMM yyyy"));
            QToolTip::showText(QCursor::pos(),
                QString("BUY @ $%1\n%2")
                    .arg(point.y(), 0, 'f', 2)
                    .arg(dtStr), this);
        }
    });

    connect(m_sellSeries, &QScatterSeries::hovered,
            [this](const QPointF& point, bool state) {
        if (state) {
            const int idx = static_cast<int>(point.x());
            QString dtStr;
            if (idx >= 0 && idx < m_barEpochMs.size())
                dtStr = QDateTime::fromMSecsSinceEpoch(m_barEpochMs.at(idx)).toString(QStringLiteral("dd MMM yyyy"));
            QToolTip::showText(QCursor::pos(),
                QString("SELL @ $%1\n%2")
                    .arg(point.y(), 0, 'f', 2)
                    .arg(dtStr), this);
        }
    });
}

int BacktestCandlestickWidget::findBarIndexForEpochMs(qint64 epochMs) const
{
    auto it = std::lower_bound(m_barEpochMs.begin(), m_barEpochMs.end(), epochMs);
    if (it != m_barEpochMs.end() && *it == epochMs)
        return static_cast<int>(std::distance(m_barEpochMs.begin(), it));
    return -1;
}

void BacktestCandlestickWidget::onChartViewRangeChanged()
{
    refitYToVisibleCandles();
    m_xLabelDebounceTimer.start(200);
}

void BacktestCandlestickWidget::onXLabelDebounceTimeout()
{
    refreshXAxisLabels();
}

void BacktestCandlestickWidget::refreshXAxisLabels()
{
    if (!m_axisX || m_barEpochMs.isEmpty())
        return;
    const int n = static_cast<int>(m_barEpochMs.size());
    if (m_axisX->categories().size() != n)
        return;

    int iMin = 0;
    int iMax = n - 1;
    if (!m_chartView->visibleCategoryBarRange(&iMin, &iMax)) {
        iMin = 0;
        iMax = n - 1;
    }

    const qint64 spanMs = qMax(qint64(1), m_barEpochMs.at(iMax) - m_barEpochMs.at(iMin));
    const double spanDays = double(spanMs) / 86400000.0;

    const QVector<int> labelIdx = pickLabelIndices(iMin, iMax, kMaxXAxisLabels);
    QSet<int>          labelPos;
    for (int idx : labelIdx)
        labelPos.insert(idx);

    QStringList newCats;
    newCats.reserve(n);

    const int nTickLabels = labelIdx.size();
    int       chosenDetail = 0;
    for (; chosenDetail < kMaxXLabelDetailLevels; ++chosenDetail) {
        QSet<QString> seen;
        bool          unique = true;
        for (int i = 0; i < n; ++i) {
            if (!labelPos.contains(i))
                continue;
            const QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_barEpochMs.at(i), QTimeZone::UTC);
            const QString   res = (i < m_barResolutions.size()) ? m_barResolutions.at(i) : QString();
            const QString   t   = formatXTickLabel(dt, res, spanDays, nTickLabels, chosenDetail);
            if (seen.contains(t)) {
                unique = false;
                break;
            }
            seen.insert(t);
        }
        if (unique)
            break;
    }
    if (chosenDetail >= kMaxXLabelDetailLevels)
        chosenDetail = kMaxXLabelDetailLevels - 1;

    for (int i = 0; i < n; ++i) {
        if (!labelPos.contains(i)) {
            newCats << (i < m_hiddenCatKeys.size() ? m_hiddenCatKeys.at(i) : hiddenCatKey(i));
            continue;
        }
        const QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_barEpochMs.at(i), QTimeZone::UTC);
        const QString   res = (i < m_barResolutions.size()) ? m_barResolutions.at(i) : QString();
        newCats << formatXTickLabel(dt, res, spanDays, nTickLabels, chosenDetail);
    }

    QSignalBlocker blocker(m_axisX);
    m_axisX->setCategories(newCats);
    m_axisX->setRange(newCats.at(iMin), newCats.at(iMax));
}

void BacktestCandlestickWidget::refitYToVisibleCandles()
{
    if (!m_axisX || !m_axisY || !m_candleSeries)
        return;

    const QStringList cats = m_axisX->categories();
    if (cats.isEmpty())
        return;

    const QString kmin = m_axisX->min();
    const QString kmax = m_axisX->max();
    if (kmin.isEmpty() || kmax.isEmpty())
        return;

    int iMin = cats.indexOf(kmin);
    int iMax = cats.indexOf(kmax);
    if (iMin < 0 || iMax < 0)
        return;
    if (iMin > iMax)
        std::swap(iMin, iMax);

    double minP = std::numeric_limits<double>::max();
    double maxP = std::numeric_limits<double>::lowest();

    const QList<QCandlestickSet*> sets = m_candleSeries->sets();
    for (QCandlestickSet* set : sets) {
        const int idx = static_cast<int>(set->timestamp());
        if (idx < iMin || idx > iMax)
            continue;
        minP = qMin(minP, set->low());
        maxP = qMax(maxP, set->high());
    }

    const QString sym = m_symbolCombo->currentText();
    for (const DbBacktestTrade& t : m_fills) {
        if (t.symbol != sym)
            continue;
        const QDateTime ts = QDateTime::fromString(t.timestamp, Qt::ISODate);
        if (!ts.isValid())
            continue;
        const qint64 msec = ts.toMSecsSinceEpoch();
        const int    barIdx = findBarIndexForEpochMs(msec);
        if (barIdx < 0 || barIdx < iMin || barIdx > iMax)
            continue;
        minP = qMin(minP, t.fillPrice);
        maxP = qMax(maxP, t.fillPrice);
    }

    if (minP >= maxP)
        return;

    double margin = qMax(1e-6, (maxP - minP) * 0.05);
    double yLo    = minP - margin;
    double yHi    = maxP + margin;
    if (minP >= 0.0 && yLo < 0.0)
        yLo = 0.0;
    QSignalBlocker blocker(m_axisY);
    m_axisY->setRange(yLo, yHi);
}

void BacktestCandlestickWidget::setData(const QMap<QString, QList<DbHistoricalBar>>& bars,
                                        const QList<DbBacktestTrade>& fills)
{
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

void BacktestCandlestickWidget::clear()
{
    m_bars.clear();
    m_fills.clear();
    m_barEpochMs.clear();
    m_barResolutions.clear();
    m_hiddenCatKeys.clear();
    m_symbolCombo->clear();
    m_candleSeries->clear();
    m_buySeries->clear();
    m_sellSeries->clear();
    m_axisX->clear();
    m_chart->setTitle(QStringLiteral("Price Chart"));
    m_chartView->clearFullRange();
}

void BacktestCandlestickWidget::onSymbolChanged(int index)
{
    Q_UNUSED(index)
    const QString sym = m_symbolCombo->currentText();
    if (!sym.isEmpty())
        buildChartForSymbol(sym);
}

void BacktestCandlestickWidget::buildChartForSymbol(const QString& symbol)
{
    m_candleSeries->clear();
    m_buySeries->clear();
    m_sellSeries->clear();
    m_axisX->clear();
    m_barEpochMs.clear();
    m_barResolutions.clear();
    m_hiddenCatKeys.clear();
    m_chart->setTitle(symbol);

    QList<DbHistoricalBar> bars = m_bars.value(symbol);
    std::sort(bars.begin(), bars.end(), barLessByTime);

    QStringList categories;
    categories.reserve(bars.size());

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = -std::numeric_limits<double>::max();

    int bi = 0;
    for (int i = 0; i < bars.size(); ++i) {
        const DbHistoricalBar& b = bars[i];
        QDateTime ts = QDateTime::fromString(b.timestamp, Qt::ISODate);
        if (!ts.isValid())
            continue;
        const QDateTime utc = ts.toUTC();
        const qint64 msec = utc.toMSecsSinceEpoch();

        categories << hiddenCatKey(bi);
        m_barEpochMs.append(msec);
        m_barResolutions.append(b.resolution);

        auto* set = new QCandlestickSet(b.open, b.high, b.low, b.close, qreal(bi));
        m_candleSeries->append(set);
        ++bi;

        minPrice = qMin(minPrice, b.low);
        maxPrice = qMax(maxPrice, b.high);
    }

    const int nBar = m_barEpochMs.size();
    m_hiddenCatKeys.resize(nBar);
    for (int i = 0; i < nBar; ++i)
        m_hiddenCatKeys[i] = hiddenCatKey(i);

    m_axisX->append(categories);

    for (const DbBacktestTrade& t : m_fills) {
        if (t.symbol != symbol)
            continue;
        QDateTime ts = QDateTime::fromString(t.timestamp, Qt::ISODate);
        if (!ts.isValid())
            continue;
        const qint64 msec = ts.toMSecsSinceEpoch();
        const int    idx  = findBarIndexForEpochMs(msec);
        if (idx < 0)
            continue;

        if (t.side == QLatin1String("BUY"))
            m_buySeries->append(qreal(idx), t.fillPrice);
        else
            m_sellSeries->append(qreal(idx), t.fillPrice);

        minPrice = qMin(minPrice, t.fillPrice);
        maxPrice = qMax(maxPrice, t.fillPrice);
    }

    const int n = m_barEpochMs.size();
    if (n >= 2 && minPrice < maxPrice) {
        const double margin = (maxPrice - minPrice) * 0.05;
        double yLo = minPrice - margin;
        double yHi = maxPrice + margin;
        if (minPrice >= 0.0 && yLo < 0.0)
            yLo = 0.0;
        m_axisY->setRange(yLo, yHi);
        m_axisX->setRange(categories.first(), categories.last());
        m_chartView->setFullCategoryRange(0, n - 1, yLo, yHi);
        refreshXAxisLabels();
        refitYToVisibleCandles();
    } else if (n == 1 && minPrice < maxPrice) {
        const double margin = (maxPrice - minPrice) * 0.05;
        double yLo = minPrice - margin;
        double yHi = maxPrice + margin;
        if (minPrice >= 0.0 && yLo < 0.0)
            yLo = 0.0;
        m_axisY->setRange(yLo, yHi);
        m_axisX->setRange(categories.first(), categories.first());
        m_chartView->setFullCategoryRange(0, 0, yLo, yHi);
        refreshXAxisLabels();
        refitYToVisibleCandles();
    } else {
        m_chartView->clearFullRange();
    }
}

} // namespace BacktestUI
