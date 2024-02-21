#include "cexecutionreport.h"

using namespace IBDataTypes;



CExecutionReport & CExecutionReport::operator=(const CExecutionReport &other)
{
    d = other.d;
    return *this;
}

CExecutionReport::CExecutionReport(void) : d(new CExecutionReportData)
{

}

CExecutionReport::CExecutionReport(qint32 _reqId, QString _ticker, qreal _avgPrice, qreal _shares) : d(new CExecutionReportData)
{
    d->m_reqId = _reqId;
}
