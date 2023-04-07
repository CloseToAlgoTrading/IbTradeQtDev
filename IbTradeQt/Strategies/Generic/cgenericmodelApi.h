#ifndef CGENERICMODELAPI_H
#define CGENERICMODELAPI_H

#include <QSharedPointer>
#include <QList>
#include <QVariantMap>
#include <QString>
#include "IBrokerAPI.h"


class CGenericModelApi;
typedef QSharedPointer<CGenericModelApi> ptrGenericModelType;

class CGenericModelApi
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


    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void setBrokerInterface(QSharedPointer<IBrokerAPI> interface) = 0;
};




#endif // CGENERICMODELAPI_H
