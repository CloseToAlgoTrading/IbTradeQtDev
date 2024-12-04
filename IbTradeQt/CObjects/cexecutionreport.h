#ifndef CEXECUTIONREPORT_H
#define CEXECUTIONREPORT_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
//------------------------------------------------------
class CExecutionReportData : public QSharedData
{
public:
    CExecutionReportData() : m_reqId(0)
        , m_Ticker("")
        , m_avgPrice(0.0)
        , m_shares(0.0)
        , m_ExecId("")
    {}

    CExecutionReportData(const CExecutionReportData &other) : QSharedData(other)
        , m_reqId(other.m_reqId)
        , m_Ticker(other.m_Ticker)
        , m_avgPrice(other.m_avgPrice)
        , m_shares(other.m_shares)
        , m_ExecId(other.m_ExecId)
    {}

    ~CExecutionReportData(){}

    qint32      m_reqId;
    QString     m_Ticker;
    qreal       m_avgPrice;
    qreal       m_shares;
    QString     m_ExecId;


};
//------------------------------------------------------
class CExecutionReport
{
public:
    CExecutionReport(void);
    CExecutionReport(qint32 _reqId,
                     QString _ticker,
                     qreal   _avgPrice,
                     qreal   _shares,
                     QString _ExecId
              );
    ~CExecutionReport(){}

    CExecutionReport(const CExecutionReport& other){
        d = other.d;
    }

    CExecutionReport & operator= (const CExecutionReport &other);

private:
    QSharedDataPointer<CExecutionReportData> d;

public:
    qint32 getId() const { return d->m_reqId; }
    QString getTicker() const { return d->m_Ticker; }
    qreal getAvgPrice() const { return d->m_avgPrice; }
    qreal getAmount() const { return d->m_shares; }
    QString getExecId() const { return d->m_ExecId; }

};
}

Q_DECLARE_METATYPE(IBDataTypes::CExecutionReport);

#endif // CEXECUTIONREPORT_H
