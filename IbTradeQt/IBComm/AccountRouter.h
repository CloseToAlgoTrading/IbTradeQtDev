#ifndef IBCOMM_ACCOUNTROUTER_H
#define IBCOMM_ACCOUNTROUTER_H

#include <QObject>
#include <QString>
#include <QMetaType>

namespace IBComm {

struct AccountSummaryData {
    Q_GADGET
    Q_PROPERTY(QString account MEMBER account)
    Q_PROPERTY(QString accountType MEMBER accountType)
    Q_PROPERTY(double buyingPower MEMBER buyingPower)
    Q_PROPERTY(double totalCashValue MEMBER totalCashValue)
    Q_PROPERTY(double netLiquidation MEMBER netLiquidation)
    Q_PROPERTY(double equityWithLoanValue MEMBER equityWithLoanValue)
    Q_PROPERTY(QString currency MEMBER currency)
public:
    QString account;
    QString accountType;
    double buyingPower = 0.0;
    double totalCashValue = 0.0;
    double netLiquidation = 0.0;
    double equityWithLoanValue = 0.0;
    QString currency;
};

class AccountRouter : public QObject {
    Q_OBJECT
public:
    explicit AccountRouter(QObject* parent = nullptr) : QObject(parent) {}

    AccountSummaryData lastSummary() const { return m_current; }

public slots:
    void onAccountSummary(const QString& account, const QString& tag,
                          const QString& value, const QString& currency) {
        m_current.account = account;
        m_current.currency = currency;
        if (tag == "AccountType")
            m_current.accountType = value;
        else if (tag == "BuyingPower")
            m_current.buyingPower = value.toDouble();
        else if (tag == "TotalCashValue")
            m_current.totalCashValue = value.toDouble();
        else if (tag == "NetLiquidation")
            m_current.netLiquidation = value.toDouble();
        else if (tag == "EquityWithLoanValue")
            m_current.equityWithLoanValue = value.toDouble();
    }

    void onAccountSummaryEnd(int reqId) {
        Q_UNUSED(reqId)
        emit accountSummaryUpdated(m_current);
    }

signals:
    void accountSummaryUpdated(const IBComm::AccountSummaryData& summary);

private:
    AccountSummaryData m_current;
};

} // namespace IBComm

Q_DECLARE_METATYPE(IBComm::AccountSummaryData)

#endif // IBCOMM_ACCOUNTROUTER_H
