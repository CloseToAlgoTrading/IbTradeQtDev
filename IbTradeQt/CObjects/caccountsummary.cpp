#include "caccountsummary.h"

using namespace IBDataTypes;

CAccountSummary & CAccountSummary::operator=(const CAccountSummary &other)
{
    d = other.d;
    return *this;
}

CAccountSummary::CAccountSummary(void) : d(new CAccountSummaryData)
{

}

CAccountSummary::CAccountSummary(QString &_accountType,
                                 QString &_account,
                                 QString &_currency,
                                 qreal _totalCashValue,
                                 qreal _buyingPower,
                                 qreal _netLiquidation,
                                 qreal _equityWithLoanValue) : d(new CAccountSummaryData)
{
    d->m_accountType = _accountType;
    d->m_account = _account;
    d->m_currency = _currency;
    d->m_totalCashValue = _totalCashValue;
    d->m_buyingPower = _buyingPower;
    d->m_netLiquidation = _netLiquidation;
    d->m_equityWithLoanValue = _equityWithLoanValue;
}

QString CAccountSummary::getAccountType() const
{
    return d->m_accountType;
}

void CAccountSummary::setAccountType(const QString &newAccountType)
{
    d->m_accountType = newAccountType;
}

QString CAccountSummary::getAccount() const
{
    return d->m_account;
}


void CAccountSummary::setAccount(const QString &newAccount)
{
    d->m_account = newAccount;
}

qreal CAccountSummary::getTotalCashValue() const
{
    return d->m_totalCashValue;
}

void CAccountSummary::setTotalCashValue(qreal newTotalCashValue)
{
    d->m_totalCashValue = newTotalCashValue;
}

qreal CAccountSummary::getBuyingPower() const
{
    return d->m_buyingPower;
}

void CAccountSummary::setBuyingPower(qreal newBuyingPower)
{
    d->m_buyingPower = newBuyingPower;
}

qreal CAccountSummary::getNetLiquidation() const
{
    return d->m_netLiquidation;
}

void CAccountSummary::setNetLiquidation(qreal newNetLiquidation)
{
    d->m_netLiquidation = newNetLiquidation;
}

qreal CAccountSummary::getEquityWithLoanValue() const
{
    return d->m_equityWithLoanValue;
}

void CAccountSummary::setEquityWithLoanValue(qreal newEquityWithLoanValue)
{
    d->m_equityWithLoanValue = newEquityWithLoanValue;
}

QString CAccountSummary::getCurrency() const
{
    return d->m_currency;
}

void CAccountSummary::setCurrency(const QString &newCurrency)
{
    d->m_currency = newCurrency;
}
