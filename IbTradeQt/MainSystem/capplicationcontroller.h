#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cbasicroot.h"
#include "IModelTreeRepository.h"
#include "SystemBackendImpl.h"

#include "cpresenter.h"
#include "ibtradesystemview.h"
#include "cmainmodel.h"
#include "Supervision/Supervisor.h"
#include "Adapters/IBOrderExecutionAdapter.h"
#include "Adapters/SqlitePositionRepository.h"
#include "Adapters/IBPositionRepositoryAdapter.h"
#include "IBComm/PositionRouter.h"
#include "IBComm/HistoricalDataRouter.h"
#include "IBComm/OrderRouter.h"
#include "IBComm/AccountRouter.h"
#include "IBComm/TimeRouter.h"
#include "IBComm/MarketDepthRouter.h"
#include <QSharedPointer>
#include <QApplication>
#include <QObject>
#include <memory>

class IModelTreeRepository;

class UiLayoutStore;

class CApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit CApplicationController(QObject *parent = nullptr);
    virtual ~CApplicationController();

    void setUpApplication(QApplication &app);

    void setPMainModel(CMainModel *newPMainModel);

public slots:
    void slotStoreModelTree();

private slots:
    void slotRestoreDefaultLayout();

private:

    CPresenter *pMainPresenter;
    CIBTradeSystemView *pMainView;
    CMainModel *pMainModel;

    CBasicRoot *m_pDataRoot;
    std::unique_ptr<IModelTreeRepository> m_modelRepo;
    SystemBackendImpl   *m_backend = nullptr;

    Supervision::Supervisor *m_pSupervisor = nullptr;
    IBOrderExecutionAdapter *m_pExecutionAdapter = nullptr;
    SqlitePositionRepository *m_pPositionRepo = nullptr;
    IBComm::PositionRouter *m_pPositionRouter = nullptr;
    IBPositionRepositoryAdapter *m_pLivePositionRepo = nullptr;
    IBComm::HistoricalDataRouter *m_pHistoricalDataRouter = nullptr;
    IBComm::OrderRouter *m_pOrderRouter = nullptr;
    IBComm::AccountRouter *m_pAccountRouter = nullptr;
    IBComm::TimeRouter *m_pTimeRouter = nullptr;
    IBComm::MarketDepthRouter *m_pMarketDepthRouter = nullptr;

    UiLayoutStore* m_layoutStore = nullptr;
};

#endif // CAPPLICATIONCONTROLLER_H
