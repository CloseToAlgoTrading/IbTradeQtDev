
#ifndef CMOMENTUM_H
#define CMOMENTUM_H


#include <QObject>
#include "cbasicstrategy_V2.h"

Q_DECLARE_LOGGING_CATEGORY(MomentumPmLog);

class cMomentum : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit cMomentum(QObject *parent = nullptr);
    virtual ~cMomentum() {};

    virtual bool start() final;
    virtual bool stop() final;

    virtual void setId(const QUuid& id) override;

    ModelType modelType() const override { return ModelType::STRATEGY_MOMENTUM; }

    /** override strategy functions **/
    //virtual void requestInitData() override;

// public:
//     DbStrategyData m_StrategyData;

public slots:
    virtual void slotDbManagerConnectionState(const bool state) override;
    // virtual void slotStrategyDataFetched(const DbStrategyData& obj, e_queryStatus state);
    // virtual void slotOpenPositionsFetched(const QList<OpenPosition> &positions, e_queryStatus state);

//protected:
//    DbModelInfo m_ModelInfo;

};

#endif // CMOMENTUM_H
