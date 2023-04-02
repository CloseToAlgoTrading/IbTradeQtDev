#ifndef CBASICSTRATEGY_H
#define CBASICSTRATEGY_H

#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>
#include "cbrokerdataprovider.h"
#include "IBrokerAPI.h"

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

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QVariantMap m_InfoMap;
    QString m_Name;
    CBrokerDataProvider m_DataProvider;

public slots:
    void onUpdateParametersSlot(const QVariantMap& parameters);
//public: signals:
//    void onUpdateParametersSignal(const QVariantMap& parameters);

};

#endif // CBASICSTRATEGY_H
