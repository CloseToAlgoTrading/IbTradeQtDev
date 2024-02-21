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
    {}

    CExecutionReportData(const CExecutionReportData &other) : QSharedData(other)
        , m_reqId(other.m_reqId)
        , m_Ticker(other.m_Ticker)
        , m_avgPrice(other.m_avgPrice)
        , m_shares(other.m_shares)
    {}

    ~CExecutionReportData(){}

    qint32      m_reqId;
    QString     m_Ticker;
    qreal       m_avgPrice;
    qreal       m_shares;


};
//------------------------------------------------------
class CExecutionReport
{
public:
    CExecutionReport(void);
    CExecutionReport(qint32 _reqId,
                     QString _ticker,
                     qreal   _avgPrice,
                     qreal   _shares
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
    // void setAccount(QString val) { d->m_account = val; }
    // Contract getContract() const { return d->m_contract; }
    // void setContract(Contract val) { d->m_contract = val; }
    // qreal  getPos() const { return d->m_pos; }
    // void setPos(qreal  val) { d->m_pos = val; }
    // qreal getAvgCost() const { return d->m_avgCost; }
    // void setAvgCost(qreal val) { d->m_avgCost = val; }

};
}

Q_DECLARE_METATYPE(IBDataTypes::CExecutionReport);

#endif // CEXECUTIONREPORT_H
