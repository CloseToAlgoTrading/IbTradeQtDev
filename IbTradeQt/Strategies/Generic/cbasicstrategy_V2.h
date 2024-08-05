#ifndef CBasicStrategy_V2_H
#define CBasicStrategy_V2_H

#include "cbasemodel.h"

class CBasicStrategy_V2 : public CBaseModel
{
public:
    explicit CBasicStrategy_V2(QObject *parent = nullptr);
    virtual ~CBasicStrategy_V2() {};

    /** override strategy functions **/
    virtual void requestInitData() override;

public:
    DbStrategyData m_StrategyData;

public slots:
    virtual void slotStrategyDataFetched(const DbStrategyData& obj, e_queryStatus state);
    virtual void slotOpenPositionsFetched(const QList<OpenPosition> &positions, e_queryStatus state);

};



#endif // CBasicStrategy_V2_H
