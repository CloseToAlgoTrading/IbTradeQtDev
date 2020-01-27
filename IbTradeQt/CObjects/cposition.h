#ifndef CPOSITION_H
#define CPOSITION_H

#include <QObject>
#include <QSharedData>
#include "GlobalDef.h"

namespace IBDataTypes
{
    //------------------------------------------------------
    class CPositionData : public QSharedData
    {
    public:
        CPositionData() : m_account("")
            , m_contract()
            , m_pos(0)
            , m_avgCost(0)
        {}

        CPositionData(const CPositionData &other) : m_account(other.m_account)
            , m_contract(other.m_contract)
            , m_pos(other.m_pos)
            , m_avgCost(other.m_avgCost)
        {}

        ~CPositionData(){}

        QString     m_account;
        Contract	m_contract;
        qreal       m_pos;
        qreal       m_avgCost;


    };
    //------------------------------------------------------
    class CPosition
    {
    public:
        CPosition(void);
        CPosition(QString _account,
            Contract _contract,
            qreal    _pos,
            qreal    _avgCost
            );
        ~CPosition(){}

        CPosition(const CPosition& other){
            d = other.d;
        }

        CPosition & operator= (const CPosition &other);

    private:
        QSharedDataPointer<CPositionData> d;

    public:
        QString getAccount() const { return d->m_account; }
        void setAccount(QString val) { d->m_account = val; }
        Contract getContract() const { return d->m_contract; }
        void setContract(Contract val) { d->m_contract = val; }
        qreal  getPos() const { return d->m_pos; }
        void setPos(qreal  val) { d->m_pos = val; }
        qreal getAvgCost() const { return d->m_avgCost; }
        void setAvgCost(qreal val) { d->m_avgCost = val; }

    };
}

Q_DECLARE_METATYPE(IBDataTypes::CPosition);

#endif // CPOSITION_H
