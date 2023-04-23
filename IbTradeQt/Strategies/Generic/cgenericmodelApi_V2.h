#ifndef CGenericModelApi_V2_H
#define CGenericModelApi_V2_H

#include <QSharedPointer>
#include <QList>
#include <QVariantMap>
#include <QString>
#include <QEnableSharedFromThis>

class IBrokerAPI;
class CGenericModelApi_V2;
typedef QSharedPointer<CGenericModelApi_V2> ptrGenericModelType_v2;

class CGenericModelApi_V2: public QEnableSharedFromThis<CGenericModelApi_V2>

{
public:

    virtual void addModel(ptrGenericModelType_v2 pModel) = 0;
    virtual void removeModel(ptrGenericModelType_v2 pModel) = 0;
    virtual QList<ptrGenericModelType_v2>& getModels() = 0;
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

    //virtual void setBrokerInterface(QSharedPointer<IBrokerAPI> interface) = 0;
};




#endif // CGenericModelApi_V2_H
