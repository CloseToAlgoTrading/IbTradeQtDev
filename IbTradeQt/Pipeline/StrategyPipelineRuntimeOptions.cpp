#include "StrategyPipelineRuntimeOptions.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPipelineConfig, "pipeline.config")

namespace Pipeline {

StrategyPipelineRuntimeOptions StrategyPipelineRuntimeOptions::fromJson(const QJsonObject& config)
{
    StrategyPipelineRuntimeOptions opts;
    
    QString policyStr = config.value("historicalReadPolicy").toString("preferCache");
    bool ok = false;
    opts.historicalReadPolicy = policyFromString(policyStr, &ok);
    
    if (!ok && !policyStr.isEmpty()) {
        qCWarning(lcPipelineConfig) << "Unknown historicalReadPolicy:" << policyStr
                                    << "— defaulting to PreferCache";
    }
    
    return opts;
}

QString StrategyPipelineRuntimeOptions::policyToString(HistoricalReadPolicy policy)
{
    switch (policy) {
    case HistoricalReadPolicy::PreferCache:       return QStringLiteral("preferCache");
    case HistoricalReadPolicy::RefreshFromSource: return QStringLiteral("refreshFromSource");
    case HistoricalReadPolicy::SourceOnly:        return QStringLiteral("sourceOnly");
    }
    return QStringLiteral("preferCache");
}

HistoricalReadPolicy StrategyPipelineRuntimeOptions::policyFromString(const QString& str, bool* ok)
{
    const QString normalized = str.trimmed().toLower();
    
    if (normalized == "prefercache") {
        if (ok) *ok = true;
        return HistoricalReadPolicy::PreferCache;
    }
    if (normalized == "refreshfromsource") {
        if (ok) *ok = true;
        return HistoricalReadPolicy::RefreshFromSource;
    }
    if (normalized == "sourceonly") {
        if (ok) *ok = true;
        return HistoricalReadPolicy::SourceOnly;
    }
    
    if (ok) *ok = false;
    return HistoricalReadPolicy::PreferCache;
}

} // namespace Pipeline
