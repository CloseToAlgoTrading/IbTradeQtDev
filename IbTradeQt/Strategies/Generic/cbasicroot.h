#ifndef CBASICROOT_H
#define CBASICROOT_H

#include "cbasicstrategy_V2.h"

class CBasicRoot : public CBasicStrategy_V2
{
public:
    explicit CBasicRoot(QObject *parent = nullptr);
    virtual ~CBasicRoot() {};

    ModelType modelType() const override { return ModelType::ROOT; }
};

#endif // CBASICROOT_H
