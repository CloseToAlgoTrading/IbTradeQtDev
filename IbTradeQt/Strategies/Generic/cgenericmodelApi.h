#ifndef CGENERICMODELAPI_H
#define CGENERICMODELAPI_H

#include <QSharedPointer>
#include <QList>
#include <QVariantMap>
#include <QString>
//#include "IBrokerAPI.h"
#include <QEnableSharedFromThis>
#include "ModelType.h"

class CGenericModelApi;
class CBrokerDataProvider;
typedef QSharedPointer<CGenericModelApi> ptrGenericModelType;

class CGenericModelApi : public QEnableSharedFromThis<CGenericModelApi>
{
public:

    virtual void addModel(ptrGenericModelType pModel) = 0;
    virtual void removeModel(ptrGenericModelType pModel) = 0;
    virtual QList<ptrGenericModelType>& getModels() = 0;
    virtual QString getName() = 0;
    virtual void setName(const QString& name) = 0;
    virtual void setParameters(const QVariantMap& parametersMap) = 0;
    virtual const QVariantMap& getParameters() = 0;

    virtual QVariantMap assetList() const = 0;
    virtual void setAssetList(const QVariantMap &newAssetList) = 0;

    virtual QVariantMap genericInfo() const = 0;
    virtual void setGenericInfo(const QVariantMap &newGenericInfo) = 0;

    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& json) = 0;

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual ModelType modelType() const = 0;
    virtual void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient) = 0;

    virtual void addSelectionModel(ptrGenericModelType pModel) = 0;
    virtual void removeSelectionModel() = 0;

    virtual void addAlphaModel(ptrGenericModelType pModel) = 0;
    virtual void removeAlphaModel() = 0;

    virtual void addRebalanceModel(ptrGenericModelType pModel) = 0;
    virtual void removeRebalanceModel() = 0;

    virtual void addRiskModel(ptrGenericModelType pModel) = 0;
    virtual void removeRiskModel() = 0;

    virtual void addExecutionModel(ptrGenericModelType pModel) = 0;
    virtual void removeExecutionModel() = 0;

    virtual ptrGenericModelType getSelectionModel() = 0;
    virtual ptrGenericModelType getAlphaModel() = 0;
    virtual ptrGenericModelType getRebalanceModel() = 0;
    virtual ptrGenericModelType getRiskModel() = 0;
    virtual ptrGenericModelType getExecutionModel() = 0;
};




#endif // CGENERICMODELAPI_H
