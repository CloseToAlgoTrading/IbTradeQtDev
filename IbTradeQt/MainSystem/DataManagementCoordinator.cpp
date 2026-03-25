#include "DataManagement/DataManagementService.h"
#include "DataManagement/DataManagementTypes.h"
#include "DataManagementCoordinator.h"
#include "DataManagement/DataManagementConstants.h"
#include "DataManagement/DataSourceIdNormalizer.h"
#include "DataManagement/ExportFilenameUtils.h"
#include "Backtest/AssetUniverseInput.h"
#include "DataManagementUI/DataManagementPanel.h"
#include "Common/NHelper.h"
#include "Common/StorageConfig.h"
#include "ibtradesystemview.h"

#include <QAbstractButton>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTextStream>
#include <QVector>

namespace {

bool scanExportConflicts(const QString& dir,
                         const QList<DataManagement::HistoricalBarsDatasetKey>& keys,
                         QVector<QString>* conflictPaths)
{
    conflictPaths->clear();
    QDir d(dir);
    for (const auto& k : keys) {
        const QString base =
            DataManagement::buildExportBasename(k.symbol, k.resolution, k.dataSourceId);
        const QString path = d.filePath(base);
        if (QFile::exists(path))
            conflictPaths->append(path);
    }
    return !conflictPaths->isEmpty();
}

} // namespace

DataManagementCoordinator::DataManagementCoordinator(QObject* parent)
    : QObject(parent)
    , m_service(new DataManagement::DataManagementService(this))
{}

DataManagementCoordinator::~DataManagementCoordinator() = default;

void DataManagementCoordinator::setView(CIBTradeSystemView* view) { m_view = view; }

void DataManagementCoordinator::setPanel(DataManagementUI::DataManagementPanel* panel)
{
    m_panel = panel;
}

QString DataManagementCoordinator::backtestDbPath() const
{
    return NHelper::getStorageConfig().backtestStore.path;
}

void DataManagementCoordinator::refreshContextAndInventory()
{
    if (!m_panel)
        return;
    DataManagement::DataManagementContext ctx;
    const StorageConfig sc = NHelper::getStorageConfig();
    ctx.appDbPath = sc.appDataStore.path;
    ctx.backtestDbPath = sc.backtestStore.path;
    ctx.activeBarsPath = sc.backtestStore.path;
    ctx.legacyPostgresConfigured = false;
    m_panel->setStorageContext(ctx);
    onPanelRefresh();
}

void DataManagementCoordinator::wireSignals()
{
    if (!m_panel || !m_service)
        return;

    connect(m_panel, &DataManagementUI::DataManagementPanel::refreshClicked, this,
            &DataManagementCoordinator::onPanelRefresh);
    connect(m_panel, &DataManagementUI::DataManagementPanel::deleteClicked, this,
            &DataManagementCoordinator::onPanelDelete);
    connect(m_panel, &DataManagementUI::DataManagementPanel::exportClicked, this,
            &DataManagementCoordinator::onPanelExport);
    connect(m_panel, &DataManagementUI::DataManagementPanel::importClicked, this,
            &DataManagementCoordinator::onPanelImport);
    connect(m_panel, &DataManagementUI::DataManagementPanel::previewClicked, this,
            &DataManagementCoordinator::onPanelPreview);
    connect(m_panel, &DataManagementUI::DataManagementPanel::coverageSyncClicked, this,
            &DataManagementCoordinator::onPanelCoverageSync);
    connect(m_panel, &DataManagementUI::DataManagementPanel::yahooBatchFetchClicked, this,
            &DataManagementCoordinator::onPanelYahooBatch);

    connect(m_service, &DataManagement::DataManagementService::barsInventoryLoaded, this,
            &DataManagementCoordinator::onInventoryLoaded);
    connect(m_service, &DataManagement::DataManagementService::barsInventoryFailed, this,
            &DataManagementCoordinator::onInventoryFailed);

    connect(m_service, &DataManagement::DataManagementService::barsDeleteFinished, this,
            &DataManagementCoordinator::onDeleteFinished);
    connect(m_service, &DataManagement::DataManagementService::barsDeleteFailed, this,
            &DataManagementCoordinator::onDeleteFailed);

    connect(m_service, &DataManagement::DataManagementService::barsPreviewReady, this,
            &DataManagementCoordinator::onPreviewReady);
    connect(m_service, &DataManagement::DataManagementService::barsPreviewFailed, this,
            &DataManagementCoordinator::onPreviewFailed);

    connect(m_service, &DataManagement::DataManagementService::barsImportFinished, this,
            &DataManagementCoordinator::onImportFinished);
    connect(m_service, &DataManagement::DataManagementService::barsImportFailed, this,
            &DataManagementCoordinator::onImportFailed);

    connect(m_service, &DataManagement::DataManagementService::barsExportFinished, this,
            &DataManagementCoordinator::onExportFinished);
    connect(m_service, &DataManagement::DataManagementService::barsExportFailed, this,
            &DataManagementCoordinator::onExportFailed);

    connect(m_service, &DataManagement::DataManagementService::barsCoverageSyncFinished, this,
            &DataManagementCoordinator::onCoverageSyncFinished);
    connect(m_service, &DataManagement::DataManagementService::barsCoverageSyncFailed, this,
            &DataManagementCoordinator::onCoverageSyncFailed);

    connect(m_service, &DataManagement::DataManagementService::barsYahooBatchImportFinished, this,
            &DataManagementCoordinator::onYahooBatchImportFinished);
    connect(m_service, &DataManagement::DataManagementService::barsYahooBatchImportFailed, this,
            &DataManagementCoordinator::onYahooBatchImportFailed);
}

void DataManagementCoordinator::onPanelRefresh()
{
    if (!m_panel || m_busy)
        return;
    m_busy = true;
    m_panel->setBusy(true);
    m_pendingInventoryOpId = m_service->requestBarsInventory(backtestDbPath());
}

void DataManagementCoordinator::onPanelDelete()
{
    if (!m_panel || m_busy)
        return;
    const auto keys = m_panel->selectedKeys();
    if (keys.isEmpty()) {
        QMessageBox::information(nullptr, QStringLiteral("Delete"), QStringLiteral("Select one or more datasets."));
        return;
    }
    if (keys.size() >= DataManagement::kLargeSelectionWarningThreshold) {
        const auto r =
            QMessageBox::question(nullptr, QStringLiteral("Delete many datasets"),
                                  QStringLiteral("Delete %1 datasets?").arg(keys.size()),
                                  QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes)
            return;
    } else {
        const auto r = QMessageBox::question(
            nullptr, QStringLiteral("Confirm delete"),
            QStringLiteral("Delete %1 dataset(s)? This cannot be undone.").arg(keys.size()),
            QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes)
            return;
    }
    m_busy = true;
    m_panel->setBusy(true);
    m_pendingDeleteOpId = m_service->requestDeleteDatasets(backtestDbPath(), keys);
}

void DataManagementCoordinator::onPanelExport()
{
    if (!m_panel || m_busy)
        return;
    const auto keys = m_panel->selectedKeys();
    if (keys.isEmpty()) {
        QMessageBox::information(nullptr, QStringLiteral("Export"),
                                 QStringLiteral("Select one or more datasets."));
        return;
    }
    if (keys.size() >= DataManagement::kLargeSelectionWarningThreshold) {
        const auto r =
            QMessageBox::question(nullptr, QStringLiteral("Export many files"),
                                  QStringLiteral("Export %1 files?").arg(keys.size()),
                                  QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes)
            return;
    }

    const QString dir = QFileDialog::getExistingDirectory(
        m_view, QStringLiteral("Export CSV directory"), QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;

    QVector<QString> conflicts;
    DataManagement::ExportOverwritePolicy pol = DataManagement::ExportOverwritePolicy::Overwrite;
    if (scanExportConflicts(dir, keys, &conflicts)) {
        QMessageBox box(m_view);
        box.setWindowTitle(QStringLiteral("Files exist"));
        box.setText(QStringLiteral("%1 file(s) already exist in the folder. Choose how to proceed.")
                        .arg(conflicts.size()));
        QPushButton* bOverwrite =
            box.addButton(QStringLiteral("Overwrite"), QMessageBox::AcceptRole);
        QPushButton* bSkip = box.addButton(QStringLiteral("Skip existing"), QMessageBox::ActionRole);
        QPushButton* bCancel = box.addButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == bCancel)
            return;
        if (box.clickedButton() == bSkip)
            pol = DataManagement::ExportOverwritePolicy::Skip;
        else if (box.clickedButton() == bOverwrite)
            pol = DataManagement::ExportOverwritePolicy::Overwrite;
        else
            return;
    }

    m_busy = true;
    m_panel->setBusy(true);
    m_pendingExportOpId = m_service->requestExport(backtestDbPath(), keys, dir, pol);
}

void DataManagementCoordinator::onPanelImport()
{
    if (!m_panel || m_busy)
        return;
    const QString path = QFileDialog::getOpenFileName(
        m_view, QStringLiteral("Import CSV"), QString(),
        QStringLiteral("CSV (*.csv);;All files (*.*)"));
    if (path.isEmpty())
        return;

    const QString res = m_panel->importResolution();
    const QString normId = DataManagement::normalizeDataSourceId(m_panel->importDataSourceIdRaw());
    if (normId.isEmpty()) {
        QMessageBox::warning(m_view, QStringLiteral("Import"),
                             QStringLiteral("Invalid dataSourceId (use a-z, 0-9, ., _, -)."));
        return;
    }

    m_busy = true;
    m_panel->setBusy(true);
    m_pendingImportOpId = m_service->requestImport(backtestDbPath(), path, res, normId);
}

void DataManagementCoordinator::onPanelPreview()
{
    if (!m_panel || m_busy)
        return;
    const auto keys = m_panel->selectedKeys();
    if (keys.size() != 1) {
        QMessageBox::information(m_view, QStringLiteral("Preview"),
                                 QStringLiteral("Select exactly one dataset."));
        return;
    }
    m_busy = true;
    m_panel->setBusy(true);
    m_pendingPreviewOpId =
        m_service->requestPreview(backtestDbPath(), keys.first(), DataManagement::kPreviewFirstRows,
                                  DataManagement::kPreviewLastRows);
}

void DataManagementCoordinator::onPanelCoverageSync()
{
    if (!m_panel || m_busy)
        return;
    const auto keys = m_panel->selectedKeys();
    if (keys.size() != 1) {
        QMessageBox::information(m_view, QStringLiteral("Coverage sync"),
                                 QStringLiteral("Select exactly one dataset."));
        return;
    }
    const DataManagement::HistoricalBarsDatasetKey k = keys.first();
    if (k.dataSourceId != QLatin1String("yahoo") || k.resolution != QLatin1String("Day1")) {
        QMessageBox::information(m_view, QStringLiteral("Coverage sync"),
                                 QStringLiteral("Coverage sync (Phase A) requires Yahoo Day1."));
        return;
    }
    m_busy = true;
    m_panel->setBusy(true);
    m_pendingCoverageSyncOpId =
        m_service->requestSyncBarsCoverage(backtestDbPath(), k, m_panel->coverageSyncFromUtc(),
                                           m_panel->coverageSyncToUtc());
}

void DataManagementCoordinator::onPanelYahooBatch()
{
    if (!m_panel || m_busy)
        return;
    const auto parsed = AssetUniverseInput::parseLine(m_panel->yahooBatchSymbolsLine());
    if (parsed.symbolOrder.isEmpty()) {
        QMessageBox::information(m_view, QStringLiteral("Yahoo import"),
                                 QStringLiteral("Enter at least one symbol (comma-separated)."));
        return;
    }
    m_busy = true;
    m_panel->setBusy(true);
    m_pendingYahooBatchOpId =
        m_service->requestBatchYahooImport(backtestDbPath(), parsed.symbolOrder,
                                           m_panel->coverageSyncFromUtc(), m_panel->coverageSyncToUtc());
}

void DataManagementCoordinator::onInventoryLoaded(quint64 operationId,
                                                  QList<DataManagement::HistoricalBarsDataset> rows)
{
    if (operationId != m_pendingInventoryOpId)
        return;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QVector<DataManagement::HistoricalBarsDataset> v;
    v.reserve(rows.size());
    for (const auto& r : rows)
        v.append(r);
    if (m_panel)
        m_panel->setDatasets(v);
    if (rows.isEmpty() && m_panel)
        m_panel->setPreviewText(QStringLiteral("No historical bar datasets found."));
}

void DataManagementCoordinator::onInventoryFailed(quint64 operationId,
                                                    DataManagement::DataManagementError err)
{
    if (operationId != m_pendingInventoryOpId)
        return;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Inventory"), err.message);
}

void DataManagementCoordinator::requestInventoryAfterMutation()
{
    m_pendingInventoryOpId = m_service->requestBarsInventory(backtestDbPath());
}

void DataManagementCoordinator::onDeleteFinished(quint64 operationId)
{
    if (operationId != m_pendingDeleteOpId)
        return;
    m_pendingDeleteOpId = 0;
    m_busy = true;
    if (m_panel)
        m_panel->setBusy(true);
    requestInventoryAfterMutation();
}

void DataManagementCoordinator::onDeleteFailed(quint64 operationId, DataManagement::DataManagementError err)
{
    if (operationId != m_pendingDeleteOpId)
        return;
    m_pendingDeleteOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Delete"), err.message);
}

void DataManagementCoordinator::onPreviewReady(quint64 operationId,
                                                   DataManagement::HistoricalBarsPreview preview)
{
    if (operationId != m_pendingPreviewOpId)
        return;
    m_pendingPreviewOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    if (m_panel)
        m_panel->setPreviewText(formatPreview(preview));
}

void DataManagementCoordinator::onPreviewFailed(quint64 operationId,
                                                DataManagement::DataManagementError err)
{
    if (operationId != m_pendingPreviewOpId)
        return;
    m_pendingPreviewOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Preview"), err.message);
}

void DataManagementCoordinator::onImportFinished(quint64 operationId, DataManagement::ImportResult result)
{
    if (operationId != m_pendingImportOpId)
        return;
    m_pendingImportOpId = 0;
    QMessageBox::information(
        m_view, QStringLiteral("Import"),
        QStringLiteral("Imported %1 rows (upserted %2). Merge by primary key (INSERT OR REPLACE).")
            .arg(result.rowsParsed)
            .arg(result.rowsUpserted));
    m_busy = true;
    if (m_panel)
        m_panel->setBusy(true);
    requestInventoryAfterMutation();
}

void DataManagementCoordinator::onImportFailed(quint64 operationId, DataManagement::DataManagementError err)
{
    if (operationId != m_pendingImportOpId)
        return;
    m_pendingImportOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QString msg = err.message;
    if (err.importLine > 0)
        msg += QStringLiteral("\nLine: %1").arg(err.importLine);
    QMessageBox::warning(m_view, QStringLiteral("Import"), msg);
}

void DataManagementCoordinator::onExportFinished(quint64 operationId,
                                                 DataManagement::ExportBatchSummary summary)
{
    if (operationId != m_pendingExportOpId)
        return;
    m_pendingExportOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QString lines;
    int ok = 0, fail = 0, skip = 0;
    for (const auto& f : summary.files) {
        if (f.action == DataManagement::ExportFileAction::Skipped)
            ++skip;
        else if (f.ok)
            ++ok;
        else
            ++fail;
        lines += f.path + QStringLiteral(" — ");
        if (!f.ok)
            lines += f.errorMessage;
        else if (f.action == DataManagement::ExportFileAction::Skipped)
            lines += QStringLiteral("skipped");
        else
            lines += QStringLiteral("ok");
        lines += QLatin1Char('\n');
    }
    QMessageBox::information(
        m_view, QStringLiteral("Export"),
        QStringLiteral("Written: %1, skipped: %2, failed: %3\n\n%4")
            .arg(ok)
            .arg(skip)
            .arg(fail)
            .arg(lines));
}

void DataManagementCoordinator::onExportFailed(quint64 operationId, DataManagement::DataManagementError err)
{
    if (operationId != m_pendingExportOpId)
        return;
    m_pendingExportOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Export"), err.message);
}

void DataManagementCoordinator::onCoverageSyncFinished(quint64 operationId,
                                                       DataManagement::SyncCoverageResult result)
{
    if (operationId != m_pendingCoverageSyncOpId)
        return;
    m_pendingCoverageSyncOpId = 0;

    QString segLines;
    for (const DataManagement::SyncCoverageSegment& s : result.segments) {
        QString kindStr;
        switch (s.kind) {
        case DataManagement::SyncCoverageSegmentKind::Full:
            kindStr = QStringLiteral("Full");
            break;
        case DataManagement::SyncCoverageSegmentKind::Prefix:
            kindStr = QStringLiteral("Prefix");
            break;
        case DataManagement::SyncCoverageSegmentKind::Suffix:
            kindStr = QStringLiteral("Suffix");
            break;
        }
        segLines += kindStr + QLatin1Char(' ') + s.fromUtc.toString(Qt::ISODate) + QLatin1String(" → ")
                    + s.toUtc.toString(Qt::ISODate) + QLatin1Char('\n');
    }
    if (segLines.isEmpty())
        segLines = QStringLiteral("(cache already covered requested range — no fetch.)\n");

    QString extra;
    if (result.hasResultingRange) {
        extra = QStringLiteral("\nCached bar range (UTC): %1 → %2")
                    .arg(result.resultingMinUtc.toString(Qt::ISODate),
                         result.resultingMaxUtc.toString(Qt::ISODate));
    }

    QMessageBox::information(
        m_view, QStringLiteral("Coverage sync"),
        QStringLiteral("Bars received (HTTP): %1\nRows upserted: %2\n\nSegments:\n%3%4")
            .arg(result.barsFetched)
            .arg(result.rowsUpserted)
            .arg(segLines)
            .arg(extra));

    m_busy = true;
    if (m_panel)
        m_panel->setBusy(true);
    requestInventoryAfterMutation();
}

void DataManagementCoordinator::onCoverageSyncFailed(quint64 operationId,
                                                     DataManagement::DataManagementError err)
{
    if (operationId != m_pendingCoverageSyncOpId)
        return;
    m_pendingCoverageSyncOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Coverage sync"), err.message);
}

void DataManagementCoordinator::onYahooBatchImportFinished(quint64 operationId,
                                                             DataManagement::BatchYahooImportResult r)
{
    if (operationId != m_pendingYahooBatchOpId)
        return;
    m_pendingYahooBatchOpId = 0;

    QString body =
        QStringLiteral("Symbols requested: %1\nSucceeded: %2\nBars received (HTTP): %3\nRows upserted: %4")
            .arg(r.symbolsRequested)
            .arg(r.symbolsSucceeded)
            .arg(r.totalBarsFetched)
            .arg(r.totalRowsUpserted);

    if (!r.failedSymbols.isEmpty()) {
        QString failures;
        for (int i = 0; i < r.failedSymbols.size(); ++i) {
            const QString reason =
                i < r.failedReasons.size() ? r.failedReasons.at(i) : QString();
            failures += r.failedSymbols.at(i) + QLatin1String(": ") + reason + QLatin1Char('\n');
        }
        body += QStringLiteral("\n\nFailures:\n") + failures;
    }

    QMessageBox::information(m_view, QStringLiteral("Yahoo import"), body);

    m_busy = true;
    if (m_panel)
        m_panel->setBusy(true);
    requestInventoryAfterMutation();
}

void DataManagementCoordinator::onYahooBatchImportFailed(quint64 operationId,
                                                         DataManagement::DataManagementError err)
{
    if (operationId != m_pendingYahooBatchOpId)
        return;
    m_pendingYahooBatchOpId = 0;
    m_busy = false;
    if (m_panel)
        m_panel->setBusy(false);
    QMessageBox::warning(m_view, QStringLiteral("Yahoo import"), err.message);
}

QString DataManagementCoordinator::formatPreview(const DataManagement::HistoricalBarsPreview& p) const
{
    QString s;
    QTextStream ts(&s);
    ts << QStringLiteral("Dataset: ") << p.key.symbol << QLatin1Char(' ') << p.key.resolution
       << QLatin1Char(' ') << p.key.dataSourceId << "\n\n";
    ts << QStringLiteral("First rows (UTC):\n");
    for (const auto& r : p.firstRowsAsc) {
        ts << r.timestampUtc.toString(Qt::ISODate) << QLatin1Char(',') << r.open << QLatin1Char(',')
           << r.high << QLatin1Char(',') << r.low << QLatin1Char(',') << r.close << QLatin1Char(',')
           << r.volume << QLatin1Char('\n');
    }
    ts << QStringLiteral("\nLast rows (UTC):\n");
    for (const auto& r : p.lastRowsAsc) {
        ts << r.timestampUtc.toString(Qt::ISODate) << QLatin1Char(',') << r.open << QLatin1Char(',')
           << r.high << QLatin1Char(',') << r.low << QLatin1Char(',') << r.close << QLatin1Char(',')
           << r.volume << QLatin1Char('\n');
    }
    return s;
}
