#include "BacktestUI/PreparePreflightDialog.h"

#include "Backtest/BacktestPreFlightCoordinator.h"
#include "Backtest/HistoricalDataManager.h"
#include "Pipeline/UniverseResolver.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString coverageStatusLabel(Backtest::SymbolCoveragePlanEntry::Status s)
{
    using S = Backtest::SymbolCoveragePlanEntry::Status;
    switch (s) {
    case S::FullyCached:
        return QStringLiteral("fully cached");
    case S::NeedsFetch:
        return QStringLiteral("needs download");
    case S::PartialGap:
        return QStringLiteral("partial gap");
    }
    return QStringLiteral("unknown");
}

QString canonForCoverage(Backtest::SymbolCoveragePlanEntry::Status s)
{
    using S = Backtest::SymbolCoveragePlanEntry::Status;
    switch (s) {
    case S::FullyCached:
        return QStringLiteral("fully_cached");
    case S::NeedsFetch:
        return QStringLiteral("needs_fetch");
    case S::PartialGap:
        return QStringLiteral("partial_gap");
    }
    return QStringLiteral("needs_fetch");
}

} // namespace

PreparePreflightDialog::PreparePreflightDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("PreparePreflightDialog"));
    setWindowTitle(QStringLiteral("Prepare run"));
    setModal(true);
    resize(520, 380);
    setMinimumSize(440, 300);
}

void PreparePreflightDialog::populate(const Backtest::BacktestPreFlightResult& r,
                                      bool showDataManagementButton)
{
    m_choseDataManagement = false;

    QStringList noteLines;
    if (r.universeMode != Pipeline::UniverseResolutionResult::Mode::ExplicitStaticSymbols) {
        noteLines << QStringLiteral(
            "Universe: the pipeline does not expose a static symbol list here; "
            "Prepare only reflects explicit symbols from the run form.");
        if (!r.universeDetail.isEmpty())
            noteLines << r.universeDetail;
    }

    QLabel* noteLabel = nullptr;
    if (!noteLines.isEmpty()) {
        noteLabel = new QLabel(this);
        noteLabel->setObjectName(QStringLiteral("preparePreflightNoteLabel"));
        noteLabel->setWordWrap(true);
        noteLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        noteLabel->setText(noteLines.join(QLatin1Char('\n')));
    }

    m_table = new FilterableTableWidget(this);
    m_table->setColumnHeaders(QStringLiteral("Ticker"), QStringLiteral("Status"));

    QVector<FilterableTableWidget::Row> rows;

    for (auto it = r.yahooValidation.failedSymbolErrors.begin();
         it != r.yahooValidation.failedSymbolErrors.end(); ++it) {
        rows.append({it.key(), it.value(), QStringLiteral("yahoo_error")});
    }

    for (auto it = r.strategySymbolCoverage.begin(); it != r.strategySymbolCoverage.end(); ++it) {
        rows.append({it.key(), coverageStatusLabel(it->status), canonForCoverage(it->status)});
    }

    if (r.hasBenchmarkCoverage) {
        rows.append({QStringLiteral("Benchmark (%1)").arg(r.benchmarkSymbolForCoverage),
                     coverageStatusLabel(r.benchmarkCoverageEntry.status),
                     canonForCoverage(r.benchmarkCoverageEntry.status)});
    }

    if (rows.isEmpty()) {
        rows.append({QStringLiteral("—"), QStringLiteral("No symbols to analyze"),
                     QStringLiteral("needs_fetch")});
    }

    m_table->setRows(rows);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    box->setObjectName(QStringLiteral("preparePreflightButtonBox"));
    QPushButton* okBtn = box->button(QDialogButtonBox::Ok);
    okBtn->setText(QStringLiteral("OK"));
    connect(okBtn, &QPushButton::clicked, this, [this]() {
        m_choseDataManagement = false;
        accept();
    });

    if (showDataManagementButton) {
        QPushButton* dmBtn =
            box->addButton(QStringLiteral("Open Data Management…"), QDialogButtonBox::ActionRole);
        connect(dmBtn, &QPushButton::clicked, this, [this]() {
            m_choseDataManagement = true;
            accept();
        });
    }

    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(10);
    if (noteLabel)
        lay->addWidget(noteLabel);
    lay->addWidget(m_table, 1);
    lay->addWidget(box);
}

int PreparePreflightDialog::run(QWidget* parent,
                                const Backtest::BacktestPreFlightResult& result,
                                bool                                       showDataManagementButton,
                                bool*                                      openDataManagementOut)
{
    PreparePreflightDialog dlg(parent);
    dlg.populate(result, showDataManagementButton);
    const int code = dlg.exec();
    if (openDataManagementOut)
        *openDataManagementOut = dlg.m_choseDataManagement;
    return code;
}
