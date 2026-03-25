#include "DataManagementUI/DataManagementPanel.h"
#include "DataManagementUI/DatasetTableModel.h"
#include "DataManagement/BarResolutionConstants.h"
#include "Backtest/AssetUniverseInput.h"

#include <QAbstractTableModel>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QTimeZone>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QTime>
#include <QVBoxLayout>
#include <QSet>

namespace DataManagementUI {

namespace {

class DatasetFilterProxy : public QSortFilterProxyModel {
public:
    explicit DatasetFilterProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    void setSymbolFilter(const QString& s)
    {
        m_sym = s.trimmed();
        invalidateFilter();
    }
    void setResolutionFilter(const QString& s)
    {
        m_res = s.trimmed();
        invalidateFilter();
    }
    void setSourceFilter(const QString& s)
    {
        m_src = s.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent))
            return false;
        const QModelIndex i0 = sourceModel()->index(source_row, DatasetTableModel::ColSymbol, source_parent);
        const QModelIndex i1 = sourceModel()->index(source_row, DatasetTableModel::ColResolution, source_parent);
        const QModelIndex i2 = sourceModel()->index(source_row, DatasetTableModel::ColDataSource, source_parent);
        const QString sym = sourceModel()->data(i0).toString();
        const QString res = sourceModel()->data(i1).toString();
        const QString src = sourceModel()->data(i2).toString();
        if (!m_sym.isEmpty() && !sym.contains(m_sym, Qt::CaseInsensitive))
            return false;
        if (!m_res.isEmpty() && m_res != QStringLiteral("(all)") && res != m_res)
            return false;
        if (!m_src.isEmpty() && !src.contains(m_src, Qt::CaseInsensitive))
            return false;
        return true;
    }

private:
    QString m_sym;
    QString m_res;
    QString m_src;
};

} // namespace

DataManagementPanel::DataManagementPanel(QWidget* parent)
    : QWidget(parent)
{
    m_storageLabel = new QLabel(this);
    m_storageLabel->setWordWrap(true);
    m_storageLabel->setObjectName(QStringLiteral("DataManagementStorageLabel"));

    m_symbolFilter = new QLineEdit(this);
    m_symbolFilter->setPlaceholderText(QStringLiteral("Filter symbol…"));
    m_resolutionFilter = new QComboBox(this);
    m_resolutionFilter->addItem(QStringLiteral("(all)"));
    for (const QString& r : DataManagement::canonicalBarResolutions())
        m_resolutionFilter->addItem(r);
    m_sourceFilter = new QLineEdit(this);
    m_sourceFilter->setPlaceholderText(QStringLiteral("Filter source…"));

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(QStringLiteral("Symbol:"), this));
    filterRow->addWidget(m_symbolFilter, 1);
    filterRow->addWidget(new QLabel(QStringLiteral("Resolution:"), this));
    filterRow->addWidget(m_resolutionFilter);
    filterRow->addWidget(new QLabel(QStringLiteral("Source:"), this));
    filterRow->addWidget(m_sourceFilter, 1);

    m_model = new DatasetTableModel(this);
    m_proxy = new DatasetFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_table = new QTableView(this);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->sortByColumn(DatasetTableModel::ColSymbol, Qt::AscendingOrder);

    connect(m_symbolFilter, &QLineEdit::textChanged, this, [this](const QString& t) {
        static_cast<DatasetFilterProxy*>(m_proxy)->setSymbolFilter(t);
    });
    connect(m_resolutionFilter, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        static_cast<DatasetFilterProxy*>(m_proxy)->setResolutionFilter(t);
    });
    connect(m_sourceFilter, &QLineEdit::textChanged, this, [this](const QString& t) {
        static_cast<DatasetFilterProxy*>(m_proxy)->setSourceFilter(t);
    });

    auto* btnRow = new QHBoxLayout();
    auto* bRefresh = new QPushButton(QStringLiteral("Refresh"), this);
    auto* bDelete = new QPushButton(QStringLiteral("Delete"), this);
    auto* bExport = new QPushButton(QStringLiteral("Export…"), this);
    auto* bImport = new QPushButton(QStringLiteral("Import CSV…"), this);
    auto* bPreview = new QPushButton(QStringLiteral("Preview"), this);
    btnRow->addWidget(bRefresh);
    btnRow->addWidget(bDelete);
    btnRow->addWidget(bExport);
    btnRow->addWidget(bImport);
    btnRow->addWidget(bPreview);
    btnRow->addStretch();

    m_importResolution = new QComboBox(this);
    for (const QString& r : DataManagement::canonicalBarResolutions())
        m_importResolution->addItem(r);
    m_importDataSource = new QLineEdit(this);
    m_importDataSource->setPlaceholderText(QStringLiteral("dataSourceId (e.g. csv)"));
    auto* importRow = new QHBoxLayout();
    importRow->addWidget(new QLabel(QStringLiteral("Import resolution:"), this));
    importRow->addWidget(m_importResolution);
    importRow->addWidget(new QLabel(QStringLiteral("dataSourceId:"), this));
    importRow->addWidget(m_importDataSource, 1);

    m_coverageSyncLabel = new QLabel(QStringLiteral("Coverage sync (UTC) — Yahoo Day1 only:"), this);
    m_syncFrom = new QDateTimeEdit(this);
    m_syncTo   = new QDateTimeEdit(this);
    for (QDateTimeEdit* dt : {m_syncFrom, m_syncTo}) {
        dt->setCalendarPopup(true);
        dt->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
        dt->setTimeZone(QTimeZone::utc());
    }
    m_syncFrom->setDateTime(QDateTime(QDate(2024, 1, 1), QTime(0, 0, 0), QTimeZone::utc()));
    m_syncTo->setDateTime(QDateTime::currentDateTimeUtc());

    m_syncCoverageButton = new QPushButton(QStringLiteral("Sync coverage"), this);
    m_coverageSyncHint = new QLabel(
        QStringLiteral("Select a Yahoo Day1 row in the table above. If the symbol is missing, load data "
                       "first (e.g. CSV import). Leading/trailing coverage only — no internal hole "
                       "repair. Other sources: Phase B."),
        this);
    m_coverageSyncHint->setWordWrap(true);
    m_coverageSyncHint->setVisible(false);

    auto* coverageRow = new QHBoxLayout();
    coverageRow->addWidget(m_coverageSyncLabel);
    coverageRow->addWidget(new QLabel(QStringLiteral("From:"), this));
    coverageRow->addWidget(m_syncFrom);
    coverageRow->addWidget(new QLabel(QStringLiteral("To:"), this));
    coverageRow->addWidget(m_syncTo);
    coverageRow->addWidget(m_syncCoverageButton);

    m_yahooBatchSymbolsEdit = new QLineEdit(this);
    m_yahooBatchSymbolsEdit->setPlaceholderText(AssetUniverseInput::lineEditPlaceholder());
    m_yahooBatchSymbolsEdit->setToolTip(AssetUniverseInput::lineEditToolTip());
    m_yahooBatchFetchButton = new QPushButton(QStringLiteral("Fetch Yahoo bars"), this);
    m_yahooBatchHint =
        new QLabel(QStringLiteral("Yahoo Day1 — comma-separated symbols (same format as backtest). "
                                  "Uses the From/To UTC fields above."),
                   this);
    m_yahooBatchHint->setWordWrap(true);

    auto* yahooBatchRow = new QHBoxLayout();
    yahooBatchRow->addWidget(new QLabel(QStringLiteral("External (Yahoo):"), this));
    yahooBatchRow->addWidget(m_yahooBatchSymbolsEdit, 1);
    yahooBatchRow->addWidget(m_yahooBatchFetchButton);

    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText(QStringLiteral("Select a row and click Preview."));

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_table);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    auto* main = new QVBoxLayout(this);
    main->addWidget(m_storageLabel);
    main->addLayout(filterRow);
    main->addWidget(splitter, 1);
    main->addLayout(btnRow);
    main->addLayout(importRow);
    main->addLayout(coverageRow);
    main->addLayout(yahooBatchRow);
    main->addWidget(m_yahooBatchHint);
    main->addWidget(m_coverageSyncHint);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &DataManagementPanel::updateCoverageSyncAvailability);

    connect(bRefresh, &QPushButton::clicked, this, &DataManagementPanel::refreshClicked);
    connect(bDelete, &QPushButton::clicked, this, &DataManagementPanel::deleteClicked);
    connect(bExport, &QPushButton::clicked, this, &DataManagementPanel::exportClicked);
    connect(bImport, &QPushButton::clicked, this, &DataManagementPanel::importClicked);
    connect(bPreview, &QPushButton::clicked, this, &DataManagementPanel::previewClicked);
    connect(m_syncCoverageButton, &QPushButton::clicked, this, &DataManagementPanel::coverageSyncClicked);
    connect(m_yahooBatchFetchButton, &QPushButton::clicked, this,
            &DataManagementPanel::yahooBatchFetchClicked);
    connect(m_yahooBatchSymbolsEdit, &QLineEdit::textChanged, this,
            &DataManagementPanel::updateYahooBatchAvailability);

    rebuildStorageLabel();
    updateYahooBatchAvailability();
}

void DataManagementPanel::rebuildStorageLabel()
{
    QString t = QStringLiteral("<b>App DB:</b> %1<br/>"
                               "<b>Backtest DB:</b> %2<br/>"
                               "<b>Active path for HistoricalBars:</b> %3")
                    .arg(m_ctx.appDbPath, m_ctx.backtestDbPath, m_ctx.activeBarsPath);
    if (m_ctx.appDbPath != m_ctx.backtestDbPath) {
        t += QStringLiteral("<br/><span style='color:#c0392b;'><b>Warning:</b> app and backtest "
                            "paths differ — this panel uses the backtest path only.</span>");
    }
    m_storageLabel->setText(t);
}

void DataManagementPanel::setStorageContext(const DataManagement::DataManagementContext& ctx)
{
    m_ctx = ctx;
    rebuildStorageLabel();
}

void DataManagementPanel::setDatasets(const QVector<DataManagement::HistoricalBarsDataset>& rows)
{
    const QList<DataManagement::HistoricalBarsDatasetKey> prev = selectedKeys();
    QSet<DataManagement::HistoricalBarsDatasetKey> toRestore;
    for (const auto& k : prev)
        toRestore.insert(k);

    m_model->setDatasets(rows);
    updateCoverageSyncAvailability();
    updateYahooBatchAvailability();

    if (toRestore.isEmpty())
        return;

    QItemSelection selection;
    const int n = m_model->rowCount();
    const int lastCol = m_model->columnCount() - 1;
    for (int r = 0; r < n; ++r) {
        if (!toRestore.contains(m_model->keyAt(r)))
            continue;
        const QModelIndex srcTop = m_model->index(r, 0);
        const QModelIndex srcBot = m_model->index(r, lastCol);
        const QModelIndex proxyTop = m_proxy->mapFromSource(srcTop);
        const QModelIndex proxyBot = m_proxy->mapFromSource(srcBot);
        if (proxyTop.isValid() && proxyBot.isValid())
            selection.select(proxyTop, proxyBot);
    }
    if (!selection.isEmpty()) {
        m_table->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect
                                                        | QItemSelectionModel::Rows);
    }
}

void DataManagementPanel::setPreviewText(const QString& text)
{
    m_preview->setPlainText(text);
}

void DataManagementPanel::setBusy(bool busy)
{
    setEnabled(!busy);
}

QList<DataManagement::HistoricalBarsDatasetKey> DataManagementPanel::selectedKeys() const
{
    QList<DataManagement::HistoricalBarsDatasetKey> keys;
    const QModelIndexList idxs = m_table->selectionModel()->selectedRows();
    for (const QModelIndex& proxyIdx : idxs) {
        const QModelIndex src = m_proxy->mapToSource(proxyIdx);
        if (src.isValid())
            keys.append(m_model->keyAt(src.row()));
    }
    return keys;
}

QString DataManagementPanel::importResolution() const
{
    return m_importResolution->currentText();
}

QString DataManagementPanel::importDataSourceIdRaw() const
{
    return m_importDataSource->text();
}

QDateTime DataManagementPanel::coverageSyncFromUtc() const
{
    return m_syncFrom->dateTime().toUTC();
}

QDateTime DataManagementPanel::coverageSyncToUtc() const
{
    return m_syncTo->dateTime().toUTC();
}

QString DataManagementPanel::yahooBatchSymbolsLine() const
{
    return m_yahooBatchSymbolsEdit ? m_yahooBatchSymbolsEdit->text() : QString();
}

void DataManagementPanel::updateYahooBatchAvailability()
{
    if (!m_yahooBatchFetchButton || !m_yahooBatchSymbolsEdit)
        return;
    const bool hasSymbols = !m_yahooBatchSymbolsEdit->text().trimmed().isEmpty();
    m_yahooBatchFetchButton->setEnabled(hasSymbols && isEnabled());
}

void DataManagementPanel::updateCoverageSyncAvailability()
{
    const auto keys = selectedKeys();
    const bool yahooDay1 = keys.size() == 1 && keys.first().dataSourceId == QLatin1String("yahoo")
                           && keys.first().resolution == QLatin1String("Day1");

    if (!yahooDay1)
        m_lastCoverageSyncKey = {};

    m_syncCoverageButton->setEnabled(yahooDay1 && isEnabled());
    m_syncFrom->setEnabled(isEnabled());
    m_syncTo->setEnabled(isEnabled());
    m_coverageSyncHint->setVisible(!yahooDay1);

    if (!yahooDay1 || m_table->selectionModel()->selectedRows().size() != 1)
        return;

    const QModelIndex proxyIdx = m_table->selectionModel()->selectedRows().first();
    const QModelIndex src      = m_proxy->mapToSource(proxyIdx);
    if (!src.isValid())
        return;

    const DataManagement::HistoricalBarsDatasetKey k = m_model->keyAt(src.row());
    if (k == m_lastCoverageSyncKey)
        return;
    m_lastCoverageSyncKey = k;

    const DataManagement::HistoricalBarsDataset ds = m_model->datasetAt(src.row());
    if (ds.fromUtc.isValid() && ds.toUtc.isValid()) {
        m_syncFrom->setDateTime(ds.fromUtc);
        m_syncTo->setDateTime(ds.toUtc);
    }
    updateYahooBatchAvailability();
}

} // namespace DataManagementUI
