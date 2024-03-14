
#ifndef CBASEREBALANCEMODEL_H
#define CBASEREBALANCEMODEL_H

#include "cbasicstrategy_V2.h"
#include "dbmanager.h"

Q_DECLARE_LOGGING_CATEGORY(BasicRebalanceModelLog);

class CBaseRebalanceModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBaseRebalanceModel(QObject *parent = nullptr);
    virtual ~CBaseRebalanceModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_REBALANCE_MODEL; }

private:
    DBManager m_dbManager;

public slots:
    virtual void processData(DataListPtr data) override;
    void slotOpenPositionsFetched(const QList<OpenPosition> &positions);
};

#endif // CBASEREBALANCEMODEL_H
