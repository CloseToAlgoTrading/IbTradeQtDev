#ifndef BLOCKINSPECTORPRESENTER_H
#define BLOCKINSPECTORPRESENTER_H

#include <QObject>
#include <QJsonObject>
#include <QList>
#include "ViewModels.h"

class BlockInspectorPresenter : public QObject
{
    Q_OBJECT
public:
    explicit BlockInspectorPresenter(QObject* parent = nullptr);

    struct BlockView {
        QString displayName;
        QString category;
        QString description;
        QString scope;
        QString blockId;
        QList<VM::ParameterRow> parameters;
    };

    BlockView resolveBlock(const QJsonObject& pipelineConfig,
                           const QString& category,
                           const QString& jsonKey,
                           bool isArray, int arrayIndex) const;

    QJsonObject applyParameterEdit(const QJsonObject& pipelineConfig,
                                   const QString& jsonKey,
                                   bool isArray, int arrayIndex,
                                   const QString& paramKey,
                                   const QString& paramValue) const;

    static QString computeDiff(const QString& current, const QString& baseline);
};

#endif // BLOCKINSPECTORPRESENTER_H
