#ifndef CBASICROOT_H
#define CBASICROOT_H

#include "cbasemodel.h"

class CBasicRoot : public CBaseModel
{
public:
    explicit CBasicRoot(QObject *parent = nullptr);
    virtual ~CBasicRoot() {};

    ModelType modelType() const override { return ModelType::ROOT; }
};

#endif // CBASICROOT_H
