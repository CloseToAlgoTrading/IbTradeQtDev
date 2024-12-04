#include "ccommissionreport.h"

using namespace IBDataTypes;



CCommissionReport & CCommissionReport::operator=(const CCommissionReport &other)
{
    d = other.d;
    return *this;
}

CCommissionReport::CCommissionReport(void) : d(new CCommissionReportData)
{

}

CCommissionReport::CCommissionReport(const QString&  _ExecId,
                                     const qreal     _commission,
                                     const QString&  _currency,
                                     const qreal	  _realizedPNL,
                                     const qreal	  _yield,
                                     const qint32	  _yieldRedemptionDate
                                     ) : d(new CCommissionReportData)
{
    d->m_ExecId = _ExecId;
    d->m_commission = _commission;
    d->m_currency = _currency;
    d->m_realizedPNL = _realizedPNL;
    d->m_yield = _yield;
    d->m_yieldRedemptionDate = _yieldRedemptionDate;
}
