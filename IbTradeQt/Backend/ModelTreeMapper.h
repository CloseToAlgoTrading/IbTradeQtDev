#ifndef MODELTREEMAPPER_H
#define MODELTREEMAPPER_H

#include "ModelNodeRecord.h"
#include "cgenericmodelApi.h"
#include <QList>

class CBasicRoot;

class ModelTreeMapper
{
public:
    // Domain -> DTOs (for persisting). ROOT is excluded.
    static QList<ModelNodeRecord> toRecords(CBasicRoot* root);

    // DTOs -> Domain (for loading). Creates synthetic ROOT in memory.
    static CBasicRoot* toRoot(const QList<ModelNodeRecord>& records);

    // Single node config extraction (strips runtime info, removes "models" array)
    static QJsonObject nodeConfigJson(CGenericModelApi* node);

private:
    static void collectRecords(CGenericModelApi* node,
                               const QString& parentUuid,
                               int& sortCounter,
                               QList<ModelNodeRecord>& out);

    static ptrGenericModelType createFromRecord(const ModelNodeRecord& record);
};

#endif // MODELTREEMAPPER_H
