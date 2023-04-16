#ifndef CBasicStrategy_V2_H
#define CBasicStrategy_V2_H

#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>
#include "cbrokerdataprovider.h"
#include "cprocessingbase_v2.h"
#include "IBrokerAPI.h"
#include <QTimer>

class CBasicStrategy_V2 : public CProcessingBase_v2, public CGenericModelApi
{
public:
    explicit CBasicStrategy_V2(QObject *parent = nullptr);
    virtual ~CBasicStrategy_V2() {};

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


    virtual QVariantMap assetList() const override;
    virtual void setAssetList(const QVariantMap &newAssetList) override;

    virtual QVariantMap genericInfo() const override;
    virtual void setGenericInfo(const QVariantMap &newGenericInfo) override;

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QVariantMap m_InfoMap;
    QString m_Name;
    CBrokerDataProvider m_DataProvider;

    QVariantMap m_assetList;
    QVariantMap m_genericInfo;
    QTimer m_tmpTimer;

public slots:
    void onUpdateParametersSlot(const QVariantMap& parameters);

    void onTimeoutSlot();
//public: signals:
//    void onUpdateParametersSignal(const QVariantMap& parameters);

};

#endif // CBasicStrategy_V2_H
