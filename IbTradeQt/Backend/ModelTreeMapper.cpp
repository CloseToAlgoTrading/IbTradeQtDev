#include "ModelTreeMapper.h"
#include "cbasicroot.h"
#include "cstrategyfactory.h"
#include "cpipelinestrategyadapter.h"
#include <QJsonArray>
#include <QJsonDocument>

QJsonObject ModelTreeMapper::nodeConfigJson(CGenericModelApi* node)
{
    QJsonObject json = node->toJson();

    json.remove("models");
    json.remove("genericInfo");

    return json;
}

void ModelTreeMapper::collectRecords(CGenericModelApi* node,
                                     const QString& parentUuid,
                                     int& sortCounter,
                                     QList<ModelNodeRecord>& out)
{
    ModelNodeRecord rec;
    rec.uuid       = node->getId().toString(QUuid::WithoutBraces);
    rec.parentUuid = parentUuid;
    rec.modelType  = static_cast<int>(node->modelType());
    rec.name       = node->getName();
    rec.config     = nodeConfigJson(node);
    rec.sortOrder  = sortCounter++;
    rec.isActive   = node->getActiveStatus();
    rec.createdAt  = QDateTime::currentDateTimeUtc();
    rec.updatedAt  = rec.createdAt;
    out.append(rec);

    int childSort = 0;
    for (auto& child : node->getModels()) {
        collectRecords(child.data(), rec.uuid, childSort, out);
    }
}

QList<ModelNodeRecord> ModelTreeMapper::toRecords(CBasicRoot* root)
{
    QList<ModelNodeRecord> records;
    if (!root) return records;

    int sortCounter = 0;
    for (auto& account : root->getModels()) {
        collectRecords(account.data(), QString(), sortCounter, records);
    }
    return records;
}

ptrGenericModelType ModelTreeMapper::createFromRecord(const ModelNodeRecord& record)
{
    ModelType type = static_cast<ModelType>(record.modelType);
    ptrGenericModelType model = CStrategyFactory::createNewStrategy(type);
    if (!model) return nullptr;

    QJsonObject cfg = record.config;
    if (!cfg.contains("m_uuid"))
        cfg["m_uuid"] = record.uuid;
    if (!cfg.contains("modelType"))
        cfg["modelType"] = record.modelType;

    QJsonObject params = cfg.value("parameters").toObject();
    if (!params.contains("Name") || params["Name"].toString().isEmpty())
        params["Name"] = record.name;
    cfg["parameters"] = params;

    model->fromJson(cfg);

    return model;
}

CBasicRoot* ModelTreeMapper::toRoot(const QList<ModelNodeRecord>& records)
{
    if (records.isEmpty()) return nullptr;

    auto* root = new CBasicRoot();
    root->setId(QUuid::createUuid());

    QHash<QString, QList<ModelNodeRecord>> childrenByParent;
    for (const auto& rec : records) {
        childrenByParent[rec.parentUuid].append(rec);
    }

    for (auto& list : childrenByParent) {
        std::sort(list.begin(), list.end(),
                  [](const ModelNodeRecord& a, const ModelNodeRecord& b) {
                      return a.sortOrder < b.sortOrder;
                  });
    }

    QHash<QString, ptrGenericModelType> createdModels;

    std::function<void(const QString&, CGenericModelApi*)> buildChildren;
    buildChildren = [&](const QString& parentUuid, CGenericModelApi* parent) {
        const auto& children = childrenByParent.value(parentUuid);
        for (const auto& childRec : children) {
            ptrGenericModelType model = createFromRecord(childRec);
            if (!model) continue;

            model->setParentModel(parent);
            parent->getModels().append(model);
            createdModels.insert(childRec.uuid, model);

            buildChildren(childRec.uuid, model.data());
        }
    };

    // Top-level nodes have empty parentUuid — these are accounts
    buildChildren(QString(), root);

    return root;
}
