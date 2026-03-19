#include "BlockInspectorPresenter.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"
#include <QJsonArray>
#include <QJsonDocument>

BlockInspectorPresenter::BlockInspectorPresenter(QObject* parent)
    : QObject(parent)
{
}

BlockInspectorPresenter::BlockView BlockInspectorPresenter::resolveBlock(
    const QJsonObject& pipelineConfig,
    const QString& category,
    const QString& jsonKey,
    bool isArray, int arrayIndex) const
{
    BlockView view;
    view.category = category;

    QJsonValue val = pipelineConfig.value(jsonKey);
    QJsonObject entry;
    if (isArray) {
        QJsonArray arr = val.toArray();
        if (arrayIndex >= 0 && arrayIndex < arr.size())
            entry = arr[arrayIndex].toObject();
    } else {
        entry = val.toObject();
    }

    QString blockId = entry.value(Pipeline::Key::BlockId).toString();
    QJsonObject cfg = entry.value(Pipeline::Key::Config).toObject();
    view.blockId = blockId;

    auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
    view.displayName = desc ? desc.value().name : blockId;
    view.description = desc ? desc.value().description : QString();
    if (desc) {
        view.scope = (desc.value().scope == Pipeline::Scope::Strategy)
                         ? QStringLiteral("Strategy")
                         : QStringLiteral("Portfolio");
    }

    for (auto it = cfg.begin(); it != cfg.end(); ++it) {
        VM::ParameterRow row;
        row.key = it.key();
        row.editable = true;

        if (it.value().isArray()) {
            QJsonDocument doc(it.value().toArray());
            row.value = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        } else if (it.value().isObject()) {
            QJsonDocument doc(it.value().toObject());
            row.value = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        } else {
            row.value = it.value().toVariant().toString();
        }

        view.parameters.append(row);
    }

    return view;
}

QJsonObject BlockInspectorPresenter::applyParameterEdit(
    const QJsonObject& pipelineConfig,
    const QString& jsonKey,
    bool isArray, int arrayIndex,
    const QString& paramKey,
    const QString& paramValue) const
{
    QJsonObject config = pipelineConfig;

    QJsonValue val = config.value(jsonKey);
    QJsonObject entry;

    if (isArray) {
        QJsonArray arr = val.toArray();
        if (arrayIndex >= 0 && arrayIndex < arr.size())
            entry = arr[arrayIndex].toObject();
    } else {
        entry = val.toObject();
    }

    QJsonObject cfg = entry.value(Pipeline::Key::Config).toObject();

    QJsonDocument doc = QJsonDocument::fromJson(paramValue.toUtf8());
    if (!doc.isNull()) {
        if (doc.isArray())
            cfg[paramKey] = doc.array();
        else if (doc.isObject())
            cfg[paramKey] = doc.object();
    } else {
        bool ok;
        double num = paramValue.toDouble(&ok);
        if (ok)
            cfg[paramKey] = num;
        else
            cfg[paramKey] = paramValue;
    }

    entry[Pipeline::Key::Config] = cfg;

    if (isArray) {
        QJsonArray arr = config.value(jsonKey).toArray();
        if (arrayIndex >= 0 && arrayIndex < arr.size())
            arr[arrayIndex] = entry;
        config[jsonKey] = arr;
    } else {
        config[jsonKey] = entry;
    }

    return config;
}

QString BlockInspectorPresenter::computeDiff(const QString& current, const QString& baseline)
{
    QStringList curLines  = current.split('\n');
    QStringList baseLines = baseline.split('\n');

    QString diff;
    int maxLines = qMax(curLines.size(), baseLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString cl = (i < curLines.size())  ? curLines[i]  : QString();
        QString bl = (i < baseLines.size()) ? baseLines[i] : QString();
        if (cl == bl) {
            diff += QStringLiteral("  ") + cl + '\n';
        } else {
            if (!bl.isEmpty())
                diff += QStringLiteral("- ") + bl + '\n';
            if (!cl.isEmpty())
                diff += QStringLiteral("+ ") + cl + '\n';
        }
    }
    return diff;
}
