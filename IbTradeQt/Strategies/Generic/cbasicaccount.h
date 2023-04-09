#ifndef CBASICACCOUNT_H
#define CBASICACCOUNT_H

#include "cbasicstrategy_V2.h"


class CBasicAccount : public CBasicStrategy_V2
{
public:
    explicit CBasicAccount(QObject *parent = nullptr);
    virtual ~CBasicAccount() {};
};

#endif // CBASICACCOUNT_H
