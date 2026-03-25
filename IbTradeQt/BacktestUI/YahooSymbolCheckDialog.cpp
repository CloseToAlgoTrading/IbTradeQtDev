#include "BacktestUI/YahooSymbolCheckDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

YahooSymbolCheckDialog::YahooSymbolCheckDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("YahooSymbolCheckDialog"));
    setWindowTitle(QStringLiteral("Yahoo symbol check"));
    setModal(true);
    resize(520, 360);
    setMinimumSize(440, 280);
}

void YahooSymbolCheckDialog::populate(const QHash<QString, QString>& failedSymbolErrors)
{
    auto* intro = new QLabel(
        QStringLiteral("Yahoo validation failed for one or more symbols:"), this);
    intro->setObjectName(QStringLiteral("yahooSymbolCheckIntroLabel"));
    intro->setWordWrap(true);

    m_table = new FilterableTableWidget(this);
    m_table->setColumnHeaders(QStringLiteral("Ticker"), QStringLiteral("Message"));

    QVector<FilterableTableWidget::Row> rows;
    rows.reserve(failedSymbolErrors.size());
    for (auto it = failedSymbolErrors.begin(); it != failedSymbolErrors.end(); ++it)
        rows.append({it.key(), it.value(), QStringLiteral("yahoo_error")});
    m_table->setRows(rows);

    auto* box = new QDialogButtonBox(this);
    box->setObjectName(QStringLiteral("yahooSymbolCheckButtonBox"));
    QPushButton* cancelBtn = box->addButton(QDialogButtonBox::Cancel);
    QPushButton* contBtn   = box->addButton(QStringLiteral("Continue without failed symbols"),
                                            QDialogButtonBox::AcceptRole);
    cancelBtn->setDefault(true);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(contBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    lay->addWidget(intro);
    lay->addWidget(m_table, 1);
    lay->addWidget(box);
}

YahooSymbolCheckDialog::Choice YahooSymbolCheckDialog::run(QWidget* parent,
                                                           const QHash<QString, QString>& failedSymbolErrors)
{
    YahooSymbolCheckDialog dlg(parent);
    dlg.populate(failedSymbolErrors);
    if (dlg.exec() != QDialog::Accepted)
        return Choice::Cancelled;
    return Choice::ContinueWithoutFailed;
}
