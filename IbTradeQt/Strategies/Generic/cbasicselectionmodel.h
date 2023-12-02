
#ifndef CBASICSELECTIONMODEL_H
#define CBASICSELECTIONMODEL_H

#include "cbasicstrategy_V2.h"
#include <QStringList>

Q_DECLARE_LOGGING_CATEGORY(BasicSelectionModelLog);

class CBasicSelectionModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicSelectionModel(QObject *parent = nullptr);
    virtual ~CBasicSelectionModel() {};

public:
    //virtual bool start() final;
    //virtual bool stop() final;

    ModelType modelType() const override { return ModelType::STRATEGY_SELECTION_MODEL; }

    virtual void setParameters(const QVariantMap& parametersMap) override;

public slots:
    virtual void processData(DataListPtr data) override;

public:
    DataListPtr m_pAssetList;
};

#endif // CBASICSELECTIONMODEL_H
