#ifndef CCOMMISSIONREPORT_H
#define CCOMMISSIONREPORT_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
//------------------------------------------------------
class CCommissionReportData : public QSharedData
{
public:
    CCommissionReportData() : m_ExecId("")
        , m_commission(0.0)
        , m_currency("")
        , m_realizedPNL(0.0)
        , m_yield(0.0)
        , m_yieldRedemptionDate(0)
    {}

    CCommissionReportData(const CCommissionReportData &other) : QSharedData(other)
        , m_ExecId(other.m_ExecId)
        , m_commission(other.m_commission)
        , m_currency(other.m_currency)
        , m_realizedPNL(other.m_realizedPNL)
        , m_yield(other.m_yield)
        , m_yieldRedemptionDate(other.m_yieldRedemptionDate)
    {}

    ~CCommissionReportData(){}

    QString     m_ExecId;
    qreal		m_commission;
    QString 	m_currency;
    qreal		m_realizedPNL;
    qreal		m_yield;
    qint32		m_yieldRedemptionDate; // YYYYMMDD format
};
//------------------------------------------------------
class CCommissionReport
{
public:
    CCommissionReport(void);
    CCommissionReport(const QString&  _ExecId,
                      const qreal     _commission,
                      const QString&  _currency,
                      const qreal	  _realizedPNL,
                      const qreal	  _yield,
                      const qint32	  _yieldRedemptionDate
                     );
    ~CCommissionReport(){}

    CCommissionReport(const CCommissionReport& other){
        d = other.d;
    }

    CCommissionReport & operator= (const CCommissionReport &other);

private:
    QSharedDataPointer<CCommissionReportData> d;

public:
    QString getId() const { return d->m_ExecId; }
    qreal getCommission() const { return d->m_commission; }
    QString getCurrency() const { return d->m_currency; }
    qreal getRealizedPNL() const { return d->m_realizedPNL; }
    qreal getYield() const { return d->m_yield; }
    qint32 getYieldRedemptionDate() const { return d->m_yieldRedemptionDate; }


};
}

Q_DECLARE_METATYPE(IBDataTypes::CCommissionReport);


#endif // CCOMMISSIONREPORT_H
