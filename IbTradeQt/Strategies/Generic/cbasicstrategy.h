#ifndef CBASICSTRATEGY_H
#define CBASICSTRATEGY_H

#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>
#include "cbrokerdataprovider.h"
#include "IBrokerAPI.h"
#include "qjsonobject.h"


class CBasicStrategy : public CGenericModelApi
{
public:
    explicit CBasicStrategy();

    // CGenericModelApi interface
public:
    virtual void addModel(ptrGenericModelType pModel) override;
    virtual void removeModel(ptrGenericModelType pModel) override;
    virtual QList<ptrGenericModelType>& getModels() override;
    virtual QString getName() override;
    virtual void setName(const QString& name) override;

    virtual void setParameters(const QVariantMap& parametersMap) override;
    virtual const QVariantMap& getParameters() override;

    virtual bool start() override;
    virtual bool stop() override;

    virtual void setBrokerInterface(QSharedPointer<IBrokerAPI> interface) override;

    virtual QJsonObject toJson() const override {QJsonObject tmp; return tmp;};
    virtual void fromJson(const QJsonObject& json) override {};

    virtual QVariantMap assetList() const override;
    virtual void setAssetList(const QVariantMap &newAssetList) override;

    virtual QVariantMap genericInfo() const override;
    virtual void setGenericInfo(const QVariantMap &newGenericInfo) override;

    ModelType modelType() const override { return ModelType::STRATEGY; }

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QVariantMap m_InfoMap;
    QString m_Name;
    CBrokerDataProvider m_DataProvider;

    QVariantMap m_assetList;
    QVariantMap m_genericInfo;

public slots:
    void onUpdateParametersSlot(const QVariantMap& parameters);
//public: signals:
//    void onUpdateParametersSignal(const QVariantMap& parameters);

};

#endif // CBASICSTRATEGY_H
