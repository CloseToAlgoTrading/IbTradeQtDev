#ifndef TST_DATA_MANAGEMENT_H
#define TST_DATA_MANAGEMENT_H

#include <QObject>

class TestDataManagement : public QObject {
    Q_OBJECT
private slots:
    void repository_inventoryAndDelete();
    void repository_previewUsesTripleWhere();
    void csvImport_parseAndNormalize();
    void exportFilename_sanitizes();
    void csvExport_roundTrip_parseMatchesDbBars();

    /// Single DataManagementService lifecycle: import→inventory round-trip + error DTO fields on failures.
    void service_asyncContract_importInventoryAndErrors();

    void yahooCoveragePlanner_tableDriven_data();
    void yahooCoveragePlanner_tableDriven();
    void coverageSync_requestSyncBarsCoverage_invalidRange_emitsFailed();
    void coverageSync_nonYahoo_emitsFailed_noDbWrites();
    void coverageSync_yahooFullyCached_zeroFetch();
    void coverageSync_yahooMin5_rejects();
};

#endif
