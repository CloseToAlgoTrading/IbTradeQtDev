#include "Backtest/BacktestWorkspaceSession.h"

#include <algorithm>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

namespace Backtest::Workspace {

bool operator==(const SessionKey& a, const SessionKey& b)
{
    return a.kind == b.kind
        && a.nodeId == b.nodeId
        && a.strategyId == b.strategyId
        && a.versionId == b.versionId;
}

bool operator!=(const SessionKey& a, const SessionKey& b)
{
    return !(a == b);
}

QString sessionKeyToQString(const SessionKey& k)
{
    if (k.kind == SessionKind::LiveNode)
        return QStringLiteral("node:") + k.nodeId;
    return QStringLiteral("catalog:") + k.strategyId + QLatin1Char(':') + k.versionId;
}

bool operator==(const RunFieldsSnapshot& a, const RunFieldsSnapshot& b)
{
    return a.symbolsText == b.symbolsText
        && a.startDate == b.startDate
        && a.endDate == b.endDate
        && qFuzzyCompare(a.initialCapital + 1.0, b.initialCapital + 1.0)
        && a.benchmarkSymbol == b.benchmarkSymbol
        && a.resolution == b.resolution
        && a.fillModel == b.fillModel
        && a.fillTiming == b.fillTiming
        && qFuzzyCompare(a.slippageBps + 1.0, b.slippageBps + 1.0)
        && a.dataSourceId == b.dataSourceId;
}

QJsonValue sortJsonValue(const QJsonValue& v);

QJsonObject sortJsonKeysRecursive(const QJsonObject& obj)
{
    QJsonObject out;
    QStringList keys = obj.keys();
    std::sort(keys.begin(), keys.end());
    for (const QString& k : keys) {
        out.insert(k, sortJsonValue(obj.value(k)));
    }
    return out;
}

QJsonArray sortJsonArray(const QJsonArray& arr)
{
    QJsonArray out;
    for (const QJsonValue& v : arr)
        out.append(sortJsonValue(v));
    return out;
}

QJsonValue sortJsonValue(const QJsonValue& v)
{
    if (v.isObject())
        return QJsonValue(sortJsonKeysRecursive(v.toObject()));
    if (v.isArray())
        return QJsonValue(sortJsonArray(v.toArray()));
    return v;
}

QString canonicalJsonString(const QJsonObject& obj)
{
    const QJsonObject sorted = sortJsonKeysRecursive(obj);
    return QString::fromUtf8(
        QJsonDocument(sorted).toJson(QJsonDocument::Compact));
}

bool sessionPipelineDirty(const Session& s)
{
    return canonicalJsonString(s.workingPipeline) != canonicalJsonString(s.baselinePipeline);
}

bool sessionRunFieldsDirty(const Session& s)
{
    return !(s.workingRunFields == s.baselineRunFields);
}

bool sessionIsDirty(const Session& s)
{
    return sessionPipelineDirty(s) || sessionRunFieldsDirty(s);
}

void recomputeSessionDirty(Session& s)
{
    s.dirty = sessionIsDirty(s);
}

} // namespace Backtest::Workspace
