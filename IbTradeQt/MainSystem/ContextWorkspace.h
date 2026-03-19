#ifndef CONTEXTWORKSPACE_H
#define CONTEXTWORKSPACE_H

#include <QStackedWidget>

class QLabel;
class CGenericModelApi;
class CPipelineStrategyAdapter;
class StrategyWorkspace;
class AccountWorkspace;
class PortfolioWorkspace;

class ContextWorkspace : public QStackedWidget
{
    Q_OBJECT
public:
    explicit ContextWorkspace(QWidget *parent = nullptr);

    void showEmpty();
    void showStrategyWorkspace(CGenericModelApi* model, CGenericModelApi* parent);
    void showAccountWorkspace(CGenericModelApi* model);
    void showPortfolioWorkspace(CGenericModelApi* model, CGenericModelApi* parent);

    void showBlockInProperties(const QString& category,
                               const QString& jsonKey,
                               bool isArray, int arrayIndex);
    void restoreStrategyProperties();

    StrategyWorkspace*  strategyWorkspace()  const { return m_strategyWS; }
    AccountWorkspace*   accountWorkspace()   const { return m_accountWS; }
    PortfolioWorkspace* portfolioWorkspace() const { return m_portfolioWS; }

private:
    QWidget*            m_emptyPage;
    StrategyWorkspace*  m_strategyWS;
    AccountWorkspace*   m_accountWS;
    PortfolioWorkspace* m_portfolioWS;
};

#endif // CONTEXTWORKSPACE_H
