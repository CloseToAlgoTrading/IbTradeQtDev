
#ifndef CBASEMODEL_H
#define CBASEMODEL_H


#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>
#include "cbrokerdataprovider.h"
#include "cprocessingbase_v2.h"
#include <QTimer>
#include <QJsonObject>

class CBaseModel : public CProcessingBase_v2, public CGenericModelApi
{
public:
    explicit CBaseModel(QObject *parent = nullptr);
    virtual ~CBaseModel() {};
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

    virtual QJsonObject toJson() const override;
    virtual void fromJson(const QJsonObject& json) override;

    virtual QVariantMap assetList() const override;
    virtual void setAssetList(const QVariantMap &newAssetList) override;

    virtual QVariantMap genericInfo() const override;
    virtual void setGenericInfo(const QVariantMap &newGenericInfo) override;

    virtual ModelType modelType() const override;
    virtual void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient) override;

    virtual void addSelectionModel(ptrGenericModelType pModel) override;
    virtual void removeSelectionModel() override;

    virtual void addAlphaModel(ptrGenericModelType pModel) override;
    virtual void removeAlphaModel() override;

    virtual void addRebalanceModel(ptrGenericModelType pModel) override;
    virtual void removeRebalanceModel() override;

    virtual void addRiskModel(ptrGenericModelType pModel) override;
    virtual void removeRiskModel() override;

    virtual void addExecutionModel(ptrGenericModelType pModel) override;
    virtual void removeExecutionModel() override;

    virtual ptrGenericModelType getSelectionModel() override;
    virtual ptrGenericModelType getAlphaModel() override;
    virtual ptrGenericModelType getRebalanceModel() override;
    virtual ptrGenericModelType getRiskModel() override;
    virtual ptrGenericModelType getExecutionModel() override;

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QVariantMap m_InfoMap;
    QString m_Name;
    //CBrokerDataProvider m_DataProvider;

    QVariantMap m_assetList;
    QVariantMap m_genericInfo;
    QTimer m_tmpTimer;

    ptrGenericModelType m_SelectionModel;
    ptrGenericModelType m_AlphaModel;
    ptrGenericModelType m_RebalanceModel;
    ptrGenericModelType m_RiskModel;
    ptrGenericModelType m_ExecutionModel;

public slots:
    void onUpdateParametersSlot(const QVariantMap& parameters);

    void onTimeoutSlot();
    //public: signals:
    //    void onUpdateParametersSignal(const QVariantMap& parameters);

};

#endif // CBASEMODEL_H
