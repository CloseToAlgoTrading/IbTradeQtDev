#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cbasicroot.h"

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
#include <QSharedPointer>
#include <QApplication>
#include <QObject>

class CApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit CApplicationController(QObject *parent = nullptr);
    virtual ~CApplicationController();

    void setUpApplication(QApplication &app);

    void setPMainModel(CMainModel *newPMainModel);

private:
    void loadTreeFromFile(const QString& fileName, QSharedPointer<CBrokerDataProvider> dataProvider = nullptr);


public slots:
    void slotStoreModelTree();

private:

    CPresenter *pMainPresenter;
    CIBTradeSystemView *pMainView;
    CMainModel *pMainModel;

    CBasicRoot *m_pDataRoot;

    Supervision::Supervisor *m_pSupervisor = nullptr;
    IBOrderExecutionAdapter *m_pExecutionAdapter = nullptr;
    SqlitePositionRepository *m_pPositionRepo = nullptr;
    IBComm::PositionRouter *m_pPositionRouter = nullptr;
    IBPositionRepositoryAdapter *m_pLivePositionRepo = nullptr;
    IBComm::HistoricalDataRouter *m_pHistoricalDataRouter = nullptr;
    IBComm::OrderRouter *m_pOrderRouter = nullptr;
    IBComm::AccountRouter *m_pAccountRouter = nullptr;
};

#endif // CAPPLICATIONCONTROLLER_H
