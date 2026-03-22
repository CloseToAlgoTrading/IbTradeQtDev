#include "backtest/tst_workspace_session.h"
#include "Backtest/BacktestWorkspaceSession.h"
#include <QJsonObject>

using namespace Backtest::Workspace;

void TestWorkspaceSession::canonicalJson_isStableForKeyOrder()
{
    QJsonObject a;
    a.insert(QStringLiteral("z"), 1);
    a.insert(QStringLiteral("a"), 2);
    QJsonObject b;
    b.insert(QStringLiteral("a"), 2);
    b.insert(QStringLiteral("z"), 1);
    QCOMPARE(canonicalJsonString(a), canonicalJsonString(b));
}

void TestWorkspaceSession::sessionDirty_detectsPipelineChange()
{
    Session s;
    s.baselinePipeline = QJsonObject{{QStringLiteral("k"), 1}};
    s.workingPipeline = s.baselinePipeline;
    s.baselineRunFields = RunFieldsSnapshot{};
    s.workingRunFields = s.baselineRunFields;
    recomputeSessionDirty(s);
    QVERIFY(!s.dirty);

    s.workingPipeline.insert(QStringLiteral("x"), 2);
    recomputeSessionDirty(s);
    QVERIFY(s.dirty);
}

void TestWorkspaceSession::sessionDirty_detectsRunFieldsChange()
{
    Session s;
    s.baselinePipeline = QJsonObject{{QStringLiteral("k"), 1}};
    s.workingPipeline = s.baselinePipeline;
    RunFieldsSnapshot rf;
    rf.symbolsText = QStringLiteral("AAPL");
    s.baselineRunFields = rf;
    s.workingRunFields = rf;
    recomputeSessionDirty(s);
    QVERIFY(!s.dirty);

    s.workingRunFields.symbolsText = QStringLiteral("MSFT");
    recomputeSessionDirty(s);
    QVERIFY(s.dirty);
}

void TestWorkspaceSession::sessionKey_qHash_equality()
{
    SessionKey a;
    a.kind = SessionKind::LiveNode;
    a.nodeId = QStringLiteral("abc");
    SessionKey b = a;
    QVERIFY(a == b);
    QCOMPARE(qHash(a), qHash(b));
}
