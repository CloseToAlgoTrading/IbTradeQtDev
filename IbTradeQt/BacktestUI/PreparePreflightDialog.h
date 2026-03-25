#ifndef BACKTESTUI_PREPAREPREFLIGHTDIALOG_H
#define BACKTESTUI_PREPAREPREFLIGHTDIALOG_H

#include <QDialog>

#include "SharedUI/FilterableTableWidget.h"

namespace Backtest {
struct BacktestPreFlightResult;
}

/// Compact modal for Prepare run results: note + filterable/sortable coverage table.
class PreparePreflightDialog final : public QDialog {
    Q_OBJECT
public:
    /// Sets \p openDataManagementOut when user clicks that button. \return QDialog::Accepted.
    static int run(QWidget* parent,
                   const Backtest::BacktestPreFlightResult& result,
                   bool                                       showDataManagementButton,
                   bool*                                      openDataManagementOut);

private:
    explicit PreparePreflightDialog(QWidget* parent = nullptr);
    void populate(const Backtest::BacktestPreFlightResult& result, bool showDataManagementButton);

    FilterableTableWidget* m_table = nullptr;
    bool                   m_choseDataManagement = false;
};

#endif
