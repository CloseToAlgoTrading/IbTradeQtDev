#ifndef CBASICACCOUNT_H
#define CBASICACCOUNT_H

//#include "cbasicstrategy_V2.h"
#include "cbasemodel.h"


class CBasicAccount : public CBaseModel
{
public:
    explicit CBasicAccount(QObject *parent = nullptr);
    virtual ~CBasicAccount() {};

    virtual void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newClient) override;
    virtual void onUpdateServerConnectionStateSlot(bool state) override;

    ModelType modelType() const override { return ModelType::ACCOUNT; }


private:
    bool requestAccountInformation();
};

#endif // CBASICACCOUNT_H
