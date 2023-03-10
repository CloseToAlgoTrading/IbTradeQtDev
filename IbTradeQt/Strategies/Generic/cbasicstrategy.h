#ifndef CBASICSTRATEGY_H
#define CBASICSTRATEGY_H

#include "cgenericmodelApi.h"
#include <QList>
#include <QVariantList>

class CBasicStrategy : public CGenericModelApi
{
public:
    explicit CBasicStrategy();

    // CGenericModelApi interface
public:
    virtual bool addModel(ptrGenericModelType pModel) override;
    virtual bool removeModel(ptrGenericModelType pModel) override;
    virtual QList<ptrGenericModelType>& getModels() override;
    virtual QString getName() override;
    virtual void setName(const QString name) override;
    virtual bool start() override;
    virtual bool stop() override;

    virtual void setParameters(const QVariantMap& parametersMap) override;
    virtual const QVariantMap& getParameters() override;

protected:
    QList<ptrGenericModelType> m_Models;
    QVariantMap m_ParametersMap;
    QString m_Name;
};

#endif // CBASICSTRATEGY_H