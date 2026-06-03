#ifndef CPRESENTER_H
#define CPRESENTER_H


#include <QObject>
#include "ibtradesystemview.h"
#include "IBworker.h"
#include "cbrokerdataprovider.h"
#include "AlphaModGetTime.h"
#include <QScopedPointer>
#include "MyLogger.h"

#include "AboutDlgPresener.h"

#include "MarketDataRouter.h"
#include <QJsonObject>

class CMainModel;
class PipelineDiagramWidget;
class QDockWidget;
class SystemTreeModel;
class SystemTreeDelegate;
class ISystemBackend;
class BacktestWorkspaceCoordinator;
class StrategyManagementCoordinator;
class DataManagementCoordinator;

class CPresenter : public QObject
{
	Q_OBJECT

public:
	CPresenter(QObject *parent);
	~CPresenter();

    void addView(CIBTradeSystemView * mw);

	void MapSignals();

	void myPrintf(const char* format, ...);


	MyLogger& m_pLog;

    CIBTradeSystemView *getPIbtsView() const;


    CMainModel *getPGuiModel() const;
    void setPGuiModel(CMainModel *newPGuiModel);

    void setBackend(ISystemBackend* backend);
    ISystemBackend* backend() const { return m_backend; }

    QSharedPointer<CBrokerDataProvider> getDataProvider() const;

    /** Safe to edit storage paths / ModelStore settings (not connected, no backtest running). */
    bool storageReconfigurationAllowed() const;
    IBComm::MarketDataRouter* marketDataRouter() const { return m_pMarketDataRouter; }
    AlphaModGetTime* getWorkerAlfaTime() const { return workerAlfaTime; }

signals:
	void signalTimeReceived(long time);
	void signalClickConnect(bool isConnect);

private slots:
	void onClickMyButton();
	void onClickPairTraderButton();
    void onClickAutoDeltaButton();
    void onClickDBStoreButton();
    void onBrokerConnectionChanged(bool connected);
    void onBrokerConnectionAttemptFinished(bool connected);

private:

    QSharedPointer<CBrokerDataProvider> m_pDataProvider;
    IBComm::MarketDataRouter* m_pMarketDataRouter = nullptr;


    CIBTradeSystemView * pIbtsView;
    CMainModel * pGuiModel;
	
	QThread* threadIBClient;
	IBWorker::Worker* workerIBClient;

	QThread* threadAlfaTime;
	AlphaModGetTime* workerAlfaTime;


private slots:
    void onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    void refreshDiagramForCurrentTab();

    PipelineDiagramWidget*              m_pDiagramWidget     = nullptr;
    QDockWidget*                        m_pDiagramDock       = nullptr;
	QScopedPointer<AboutDlgPresener> pAboutDlgPresenter;

    ISystemBackend*     m_backend             = nullptr;
    bool                m_brokerConnected     = false;
    bool                m_connectionRequested = false;
    SystemTreeModel*    m_pSystemTreeModel    = nullptr;
    SystemTreeDelegate* m_pSystemTreeDelegate = nullptr;

    BacktestWorkspaceCoordinator*     m_backtestCoord  = nullptr;
    StrategyManagementCoordinator*    m_stratMgmtCoord = nullptr;
    DataManagementCoordinator*        m_dataMgmtCoord  = nullptr;
    int                                 m_lastMainTabIndex = 0;
};

#endif // CPRESENTER_H
