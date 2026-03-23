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

ModelDataList modelDataFromTargetPositions(const QVector<TargetPosition>& targets);

QVector<ExecutionIntent> executionIntentsFromModelData(
    const ModelDataList& data,
    const QMap<QString, double>& currentPositions,
    const QString& correlationId,
    const QDateTime& eventTime);

} // namespace SemanticMapping
} // namespace Pipeline

#endif // PIPELINE_SEMANTICMODELDATAMAPPER_H
