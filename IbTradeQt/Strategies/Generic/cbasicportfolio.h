#ifndef CBASICPORTFOLIO_H
#define CBASICPORTFOLIO_H

#include "cbasemodel.h"

class CBasicPortfolio : public CBaseModel
{
public:
    explicit CBasicPortfolio(QObject *parent = nullptr);
    virtual ~CBasicPortfolio() {};

    ModelType modelType() const override { return ModelType::PORTFOLIO; }
    QVariantMap assetList() const override;
};

#endif // CBASICPORTFOLIO_H
