#ifndef PIPELINE_STRATEGYPIPELINERUNTIMEOPTIONS_H
#define PIPELINE_STRATEGYPIPELINERUNTIMEOPTIONS_H

#include "HistoricalReadPolicy.h"
#include <QJsonObject>
#include <QString>

namespace Pipeline {

/// Typed runtime configuration options for strategy pipelines.
/// Parsed once from pipeline JSON root config; used by controller, session, and runner.
struct StrategyPipelineRuntimeOptions {
    HistoricalReadPolicy historicalReadPolicy = HistoricalReadPolicy::PreferCache;

    /// Parse from pipeline config JSON. 
    /// Parsing is case-insensitive for convenience.
    /// Unknown/invalid values default to PreferCache with warning.
    static StrategyPipelineRuntimeOptions fromJson(const QJsonObject& config);
    
    /// Convert policy enum to canonical string (for logging, serialization, debugging).
    static QString policyToString(HistoricalReadPolicy policy);
    
    /// Parse string to policy enum (case-insensitive). Returns PreferCache for unknown values.
    static HistoricalReadPolicy policyFromString(const QString& str, bool* ok = nullptr);
};

} // namespace Pipeline

#endif // PIPELINE_STRATEGYPIPELINERUNTIMEOPTIONS_H
