#ifndef PIPELINE_SEMANTICMODELDATAMAPPER_H
#define PIPELINE_SEMANTICMODELDATAMAPPER_H

#include "SemanticTypes.h"
#include "Contracts.h"
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace Pipeline {
namespace SemanticMapping {

/// Build a model-data list from symbols (direction undefined, zero amount).
ModelDataList buildModelDataFromSymbols(const QVector<QString>& symbols);

QStringList symbolsFromModelData(const ModelDataList& data);

QVector<Signal> signalsFromModelData(
    const ModelDataList& data,
    const QString& correlationId,
    const QString& alphaBlockId = QString());

/// Maps pipeline signals back to unified rows (confidence from Signal; amount from suggestedQuantity when set).
ModelDataList modelDataFromSignals(const QVector<Signal>& inputSignals);

QVector<TargetPosition> targetPositionsFromModelData(
    const ModelDataList& data,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId);

/// Encodes **execution delta** (target − current) per symbol as UP/DOWN + |delta| in `UnifiedModelData`.
/// This reuses `eDirection` for **signed trade delta**, not alpha signal direction — alpha intent lives in
/// upstream rows only; downstream risk/execution should treat these rows as **order delta**, not “alpha UP/DOWN”.
/// Prefer a future refactor: pass `TargetPosition` through risk/execution without this round-trip (see
/// `IRebalanceBlock::processSemantic`).
ModelDataList modelDataFromTargetPositions(const QVector<TargetPosition>& targets);

QVector<ExecutionIntent> executionIntentsFromModelData(
    const ModelDataList& data,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime);

/// Build execution intents from absolute targets vs \a currentPositions. Sells (negative qty) are
/// ordered before buys so liquidation/reduction frees notional before buys in sequential simulation.
QVector<ExecutionIntent> executionIntentsFromTargetPositions(
    const QVector<TargetPosition>& targets,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime);

/// One row per normalized symbol (uppercase trim); later entries win. Floors negative targets to 0 (long-only).
QVector<TargetPosition> mergeTargetPositionsBySymbolLastWins(const QVector<TargetPosition>& targets);

} // namespace SemanticMapping
} // namespace Pipeline

#endif // PIPELINE_SEMANTICMODELDATAMAPPER_H
