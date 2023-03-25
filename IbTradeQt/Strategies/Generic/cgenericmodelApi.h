#ifndef CGENERICMODELAPI_H
#define CGENERICMODELAPI_H

#include <QSharedPointer>
#include <QList>
#include <QVariantMap>
#include <QString>

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

    virtual bool start() = 0;
    virtual bool stop() = 0;
};




#endif // CGENERICMODELAPI_H
