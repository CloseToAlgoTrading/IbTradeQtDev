#ifndef PORTFOLIOMODELDEFINES_H
#define PORTFOLIOMODELDEFINES_H

#define PM_ITEM_ACCOUNTS   (0x0010u)
#define PM_ITEM_ACCOUNT    (0x0020u)
#define PM_ITEM_PORTFOLIO  (0x0030u)
#define PM_ITEM_STRATEGIES (0x0040u)
#define PM_ITEM_STRATEGY   (0x0050u)
#define PM_ITEM_PARAMETERS (0x0060u)
#define PM_ITEM_PARAMETER  (0x0070u)

#define PM_ITEM_SELECTION_MODEL (0x0080u)
#define PM_ITEM_ALFA_MODEL      (0x0090u)
#define PM_ITEM_REBALANCE_MODEL (0x00A0u)
#define PM_ITEM_EXECUTION_MODEL (0x00B0u)
#define PM_ITEM_RISK_MODEL      (0x00C0u)



#define PT_ITEM_ACTIVATION (0x0F00u)
#define PM_ITEM_ACCOUNT_ACTIVATION    (PT_ITEM_ACTIVATION + PM_ITEM_ACCOUNT)
#define PM_ITEM_PARAMETERS_ACTIVATION (PT_ITEM_ACTIVATION + PM_ITEM_PARAMETERS)
#define PM_ITEM_PARAMETER_ACTIVATION  (PT_ITEM_ACTIVATION + PM_ITEM_PARAMETER)
#define PM_ITEM_STRATEGIES_ACTIVATION (PT_ITEM_ACTIVATION + PM_ITEM_STRATEGIES)
#define PM_ITEM_STRATEGY_ACTIVATION   (PT_ITEM_ACTIVATION + PM_ITEM_STRATEGY)
#define PM_ITEM_PORTFOLIO_ACTIVATION  (PT_ITEM_ACTIVATION + PM_ITEM_PORTFOLIO)
#define PM_ITEM_ACCOUNTS_ACTIVATION   (PT_ITEM_ACTIVATION + PM_ITEM_ACCOUNTS)

#define PM_ITEM_ID_MASK    (0x0FFFu)

#endif // PORTFOLIOMODELDEFINES_H
