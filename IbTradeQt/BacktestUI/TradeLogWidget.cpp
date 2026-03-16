#include "BacktestUI/TradeLogWidget.h"
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QColor>

namespace BacktestUI {

static const QColor kBuyColor  { 0xd4, 0xed, 0xda }; // light green
static const QColor kSellColor { 0xf8, 0xd7, 0xda }; // light red

TradeLogWidget::TradeLogWidget(QWidget* parent)
    : QWidget(parent)
{
    setupModel();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
}

void TradeLogWidget::setupModel() {
    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({
        QStringLiteral("Date"),
        QStringLiteral("Symbol"),
        QStringLiteral("Side"),
        QStringLiteral("Qty"),
        QStringLiteral("Fill Price"),
        QStringLiteral("P&L (approx)")
    });

    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
}

void TradeLogWidget::clear() {
    m_model->removeRows(0, m_model->rowCount());
}

void TradeLogWidget::setFills(const QVector<Backtest::FilledOrder>& fills) {
    clear();
    m_model->setRowCount(fills.size());

    for (int i = 0; i < fills.size(); ++i) {
        const auto& f = fills[i];
        const bool isBuy = f.quantity > 0;
        const QColor rowColor = isBuy ? kBuyColor : kSellColor;

        auto setItem = [&](int col, const QString& text) {
            auto* item = new QStandardItem(text);
            item->setBackground(rowColor);
            item->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            m_model->setItem(i, col, item);
        };

        setItem(ColDate,   f.timestamp.toLocalTime().toString("yyyy-MM-dd hh:mm"));
        setItem(ColSymbol, f.symbol);
        setItem(ColSide,   isBuy ? QStringLiteral("BUY") : QStringLiteral("SELL"));
        setItem(ColQty,    QString::number(std::abs(f.quantity), 'f', 0));
        setItem(ColPrice,  QString("$%1").arg(f.fillPrice, 0, 'f', 2));
        setItem(ColPnl,    QStringLiteral("—")); // P&L requires round-trip tracking; left to future
    }
}

void TradeLogWidget::setDbTrades(const QList<DbBacktestTrade>& trades) {
    clear();
    m_model->setRowCount(trades.size());

    for (int i = 0; i < trades.size(); ++i) {
        const auto& t = trades[i];
        const bool isBuy = (t.side == QLatin1String("BUY"));
        const QColor rowColor = isBuy ? kBuyColor : kSellColor;

        auto setItem = [&](int col, const QString& text) {
            auto* item = new QStandardItem(text);
            item->setBackground(rowColor);
            item->setData(Qt::AlignCenter, Qt::TextAlignmentRole);
            m_model->setItem(i, col, item);
        };

        QDateTime ts = QDateTime::fromString(t.timestamp, Qt::ISODate).toLocalTime();
        setItem(ColDate,   ts.toString("yyyy-MM-dd hh:mm"));
        setItem(ColSymbol, t.symbol);
        setItem(ColSide,   t.side);
        setItem(ColQty,    QString::number(t.quantity, 'f', 0));
        setItem(ColPrice,  QString("$%1").arg(t.fillPrice, 0, 'f', 2));
        setItem(ColPnl,    QStringLiteral("—"));
    }
}

} // namespace BacktestUI
