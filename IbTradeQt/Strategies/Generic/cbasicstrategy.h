#ifndef CBASICSTRATEGY_H
#define CBASICSTRATEGY_H

#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>
#include "cbrokerdataprovider.h"
#include "IBrokerAPI.h"


struct strategyGenericInfoType
{
    strategyGenericInfoType() {
        pnl = 0.0f;
        pnl_pct = 0.0f;
    }

    qfloat16 pnl;
    qfloat16 pnl_pct;
};

struct assetInfoType
{
    assetInfoType() {
        name = "";
    }

    QString name;
    strategyGenericInfoType pnlInfo;
};


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

    QList<assetInfoType> assetList() const;
    void setAssetList(const QList<assetInfoType> &newAssetList);

    strategyGenericInfoType genericInfo() const;
    void setGenericInfo(const strategyGenericInfoType &newGenericInfo);

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QVariantMap m_InfoMap;
    QString m_Name;
    CBrokerDataProvider m_DataProvider;

    QList<assetInfoType> m_assetList;
    strategyGenericInfoType m_genericInfo;

public slots:
    void onUpdateParametersSlot(const QVariantMap& parameters);
//public: signals:
//    void onUpdateParametersSignal(const QVariantMap& parameters);

};


Q_DECLARE_METATYPE(strategyGenericInfoType)
Q_DECLARE_METATYPE(assetInfoType)

#endif // CBASICSTRATEGY_H
