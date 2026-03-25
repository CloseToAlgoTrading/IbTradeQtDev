#ifndef BACKTESTUI_YAHOO_SYMBOL_CHECK_DIALOG_H
#define BACKTESTUI_YAHOO_SYMBOL_CHECK_DIALOG_H

#include <QDialog>
#include <QHash>

#include "SharedUI/FilterableTableWidget.h"

/// Modal for Yahoo lightweight validation failures before Run: table + Cancel / Continue without failed symbols.
class YahooSymbolCheckDialog final : public QDialog {
    Q_OBJECT
public:
    enum class Choice { Cancelled, ContinueWithoutFailed };

    /// Shows failed symbols in a filterable/sortable table.
    static Choice run(QWidget* parent, const QHash<QString, QString>& failedSymbolErrors);

private:
    explicit YahooSymbolCheckDialog(QWidget* parent = nullptr);
    void populate(const QHash<QString, QString>& failedSymbolErrors);

    FilterableTableWidget* m_table = nullptr;
};

#endif
