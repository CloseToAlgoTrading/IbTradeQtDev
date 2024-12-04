#ifndef CAccountSummary_H
#define CAccountSummary_H

#include <QObject>
#include <QSharedData>

namespace IBDataTypes
{
//------------------------------------------------------
class CAccountSummaryData : public QSharedData
{
public:
    CAccountSummaryData() :   m_accountType("")
                            , m_account("")
                            , m_currency("")
                            , m_totalCashValue(0.0f)
                            , m_buyingPower(0.0f)
                            , m_netLiquidation(0.0f)
                            , m_equityWithLoanValue(0.0f)
    {}

    CAccountSummaryData(const CAccountSummaryData &other) : QSharedData(other)
        , m_accountType(other.m_accountType)
        , m_account(other.m_account)
        , m_currency(other.m_currency)
        , m_totalCashValue(other.m_totalCashValue)
        , m_buyingPower(other.m_buyingPower)
        , m_netLiquidation(other.m_netLiquidation)
        , m_equityWithLoanValue(other.m_equityWithLoanValue)
    {}

    ~CAccountSummaryData(){}

    QString     m_accountType;
    QString     m_account;
    QString 	m_currency;
    qreal		m_totalCashValue;
    qreal		m_buyingPower;
    qreal       m_netLiquidation;
    qreal       m_equityWithLoanValue;
};
//------------------------------------------------------
class CAccountSummary
{
public:
    CAccountSummary(void);
    CAccountSummary(QString&    _accountType,
                    QString&    _account,
                    QString& 	_currency,
                    qreal		_totalCashValue,
                    qreal		_buyingPower,
                    qreal       _netLiquidation,
                    qreal       _equityWithLoanValue
                      );
    ~CAccountSummary(){}

    CAccountSummary(const CAccountSummary& other){
        d = other.d;
    }

    CAccountSummary & operator= (const CAccountSummary &other);

private:
    QSharedDataPointer<CAccountSummaryData> d;

public:
    QString getAccountType() const;
    void setAccountType(const QString &newAccountType);
    QString getAccount() const;
    void setAccount(const QString &newAccount);
    QString getCurrency() const;
    void setCurrency(const QString &newCurrency);
    qreal getTotalCashValue() const;
    void setTotalCashValue(qreal newTotalCashValue);
    qreal getBuyingPower() const;
    void setBuyingPower(qreal newBuyingPower);
    qreal getNetLiquidation() const;
    void setNetLiquidation(qreal newNetLiquidation);
    qreal getEquityWithLoanValue() const;
    void setEquityWithLoanValue(qreal newEquityWithLoanValue);

};
}

Q_DECLARE_METATYPE(IBDataTypes::CAccountSummary);


#endif // CAccountSummary_H
