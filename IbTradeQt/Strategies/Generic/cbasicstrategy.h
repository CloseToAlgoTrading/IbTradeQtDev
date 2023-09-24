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

    virtual QJsonObject toJson() const override {QJsonObject tmp; return tmp;};
    virtual void fromJson(const QJsonObject& json) override {Q_UNUSED(json)};

    virtual QVariantMap assetList() const override;
    virtual void setAssetList(const QVariantMap &newAssetList) override;

    virtual QVariantMap genericInfo() const override;
    virtual void setGenericInfo(const QVariantMap &newGenericInfo) override;

    ModelType modelType() const override { return ModelType::STRATEGY; }
    void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient) override {Q_UNUSED(newClient)};

    virtual void addSelectionModel(ptrGenericModelType pModel) override {};
    virtual void removeSelectionModel() override{};

    virtual void addAlphaModel(ptrGenericModelType pModel) override{};
    virtual void removeAlphaModel() override{};

    virtual void addRebalanceModel(ptrGenericModelType pModel) override{};
    virtual void removeRebalanceModel() override{};

    virtual void addRiskModel(ptrGenericModelType pModel) override{};
    virtual void removeRiskModel() override{};

    virtual void addExecutionModel(ptrGenericModelType pModel) override{};
    virtual void removeExecutionModel() override{};

    virtual ptrGenericModelType getSelectionModel() override{return 0;};
    virtual ptrGenericModelType getAlphaModel() override{return 0;};
    virtual ptrGenericModelType getRebalanceModel() override{return 0;};
    virtual ptrGenericModelType getRiskModel() override{return 0;};
    virtual ptrGenericModelType getExecutionModel() override{return 0;};
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
