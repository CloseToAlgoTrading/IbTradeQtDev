#ifndef BACKTEST_BACKTESTWORKSPACESESSION_H
#define BACKTEST_BACKTESTWORKSPACESESSION_H

#include <QJsonObject>
#include <QString>
#include <QDateTime>
#include <QtGlobal>

namespace Backtest::Workspace {

enum class SessionKind {
    LiveNode,
    CatalogVersion
};

struct SessionKey {
    SessionKind kind = SessionKind::LiveNode;
    QString     nodeId;
    QString     strategyId;
    QString     versionId;
};

bool operator==(const SessionKey& a, const SessionKey& b);
bool operator!=(const SessionKey& a, const SessionKey& b);

QString sessionKeyToQString(const SessionKey& k);

inline uint qHash(const SessionKey& key, uint seed = 0) noexcept
{
    return ::qHash(static_cast<int>(key.kind), seed)
        ^ ::qHash(key.nodeId, seed + 1)
        ^ ::qHash(key.strategyId, seed + 2)
        ^ ::qHash(key.versionId, seed + 3);
}

struct RunFieldsSnapshot {
    QString     symbolsText;
    QDateTime   startDate;
    QDateTime   endDate;
    double      initialCapital = 100'000.0;
    QString     benchmarkSymbol;
    QString     resolution;
    QString     fillModel;
    QString     fillTiming;
    double      slippageBps    = 1.0;
    QString     dataSourceId;
};

bool operator==(const RunFieldsSnapshot& a, const RunFieldsSnapshot& b);

struct Session {
    SessionKey        key;
    QString           displayName;
    QString           portfolioPath;
    QString           strategyDefId;
    int               strategyVersion = 1;
    QString           catalogVersionId;

    QJsonObject       baselinePipeline;
    QJsonObject       workingPipeline;

    RunFieldsSnapshot baselineRunFields;
    RunFieldsSnapshot workingRunFields;

    bool              dirty           = false;
    bool              resultsStale    = false;
    QString           lastRunId;

    QString           catalogStrategyIdForCatalogPreview;
};

QJsonObject sortJsonKeysRecursive(const QJsonObject& obj);

QString canonicalJsonString(const QJsonObject& obj);

bool sessionPipelineDirty(const Session& s);
bool sessionRunFieldsDirty(const Session& s);
bool sessionIsDirty(const Session& s);

void recomputeSessionDirty(Session& s);

} // namespace Backtest::Workspace

#endif // BACKTEST_BACKTESTWORKSPACESESSION_H
