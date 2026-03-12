#include <QCoreApplication>
#include <QtTest>

#include "phase1/tst_contracts.h"
#include "phase1/tst_merge_policies.h"
#include "phase1/tst_market_data_router.h"
#include "phase1/tst_block_interfaces.h"
#include "phase1/tst_expected.h"
#include "phase1/tst_scope.h"
#include "phase2/tst_replay.h"
#include "phase2/tst_adapters.h"
#include "phase2/tst_integration.h"
#include "phase3/tst_block_registry.h"
#include "phase3/tst_pipeline_runner.h"
#include "phase4/tst_supervision.h"
#include "phase5/tst_observability.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int status = 0;

    // Phase 1
    { TestContracts tc;        status |= QTest::qExec(&tc, argc, argv); }
    { TestMergePolicies tc;    status |= QTest::qExec(&tc, argc, argv); }
    { TestMarketDataRouter tc; status |= QTest::qExec(&tc, argc, argv); }
    { TestBlockInterfaces tc;  status |= QTest::qExec(&tc, argc, argv); }
    { TestExpected tc;         status |= QTest::qExec(&tc, argc, argv); }
    { TestScope tc;            status |= QTest::qExec(&tc, argc, argv); }

    // Phase 2
    { TestReplay tc;           status |= QTest::qExec(&tc, argc, argv); }
    { TestAdapters tc;         status |= QTest::qExec(&tc, argc, argv); }
    { TestIntegration tc;      status |= QTest::qExec(&tc, argc, argv); }

    // Phase 3
    { TestBlockRegistry tc;    status |= QTest::qExec(&tc, argc, argv); }
    { TestPipelineRunner tc;   status |= QTest::qExec(&tc, argc, argv); }

    // Phase 4
    { TestSupervision tc;      status |= QTest::qExec(&tc, argc, argv); }

    // Phase 5
    { TestObservability tc;    status |= QTest::qExec(&tc, argc, argv); }

    return status;
}
