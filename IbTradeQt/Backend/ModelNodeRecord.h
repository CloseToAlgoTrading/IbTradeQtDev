#ifndef MODELNODERECORD_H
#define MODELNODERECORD_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>

struct ModelNodeRecord {
    QString uuid;
    QString parentUuid;   // empty for top-level accounts (ROOT is not persisted)
    int modelType = 0;    // maps to ModelType enum
    QString name;
    QJsonObject config;   // stable config only (parameters, info, assetList, pipelineConfig)
    int sortOrder = 0;
    bool isActive = true;
    QDateTime createdAt;
    QDateTime updatedAt;
};

#endif // MODELNODERECORD_H
