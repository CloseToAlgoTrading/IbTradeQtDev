#ifndef BACKTESTUI_TRADELOGWIDGET_H
#define BACKTESTUI_TRADELOGWIDGET_H

// TradeLogWidget — sortable table of all fills from a backtest run.
//
// Columns: Date | Symbol | Side | Qty | Fill Price | P&L (approx)
// Appearance: global QTableView QSS (+ objectName BacktestTradeLogTable for overrides).
// The model is sortable by clicking any column header.

#include <QWidget>
#include <QVector>
#include "Backtest/FilledOrder.h"
#include "DB/dbdatatypes.h"

class QTableView;
class QStandardItemModel;

namespace BacktestUI {

class TradeLogWidget : public QWidget {
    Q_OBJECT

public:
    explicit TradeLogWidget(QWidget* parent = nullptr);

    // Populate from in-memory fills (used immediately after a run).
    void setFills(const QVector<Backtest::FilledOrder>& fills);

    // Populate from DB trade records (used when loading a past run).
    void setDbTrades(const QList<DbBacktestTrade>& trades);

    // Clear all rows.
    void clear();

private:
    void setupModel();

    QTableView*          m_table = nullptr;
    QStandardItemModel*  m_model = nullptr;

    static constexpr int ColDate      = 0;
    static constexpr int ColSymbol    = 1;
    static constexpr int ColSide      = 2;
    static constexpr int ColQty       = 3;
    static constexpr int ColPrice     = 4;
    static constexpr int ColPnl       = 5;
    static constexpr int ColCount     = 6;
};

} // namespace BacktestUI

#endif // BACKTESTUI_TRADELOGWIDGET_H
