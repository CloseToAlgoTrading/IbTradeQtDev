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
#include "Backtest/BacktestDataTypes.h"
#include <QJsonObject>

class CMainModel;
class PipelineDiagramWidget;
class QDockWidget;
class SystemTreeModel;
class SystemTreeDelegate;
class ISystemBackend;

namespace BacktestUI {
    class BacktestWorkspaceDock;
    class BacktestStrategySelector;
}
namespace Backtest { class BacktestController; }

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

    // Backtest workspace slots
    void onOpenInBacktestWorkspace(const QString& strategyId,
                                   const QString& displayName,
                                   const QString& portfolioPath,
                                   const QJsonObject& pipelineConfig);
    void onLoadRun(const QString& runId);
    void onBacktestFinished(const Backtest::BacktestLoadedRun& run);
    void onBacktestFailed(const QString& reason);

    void refreshBacktestStrategies();
    void refreshStrategyCatalog();
    void openCatalogVersionInBacktest(const QString& catalogStrategyId,
                                      const QString& catalogVersionId);

private:
    PipelineDiagramWidget*              m_pDiagramWidget     = nullptr;
    QDockWidget*                        m_pDiagramDock       = nullptr;
    BacktestUI::BacktestWorkspaceDock*  m_pBacktestDock      = nullptr;
    Backtest::BacktestController*       m_pBacktestController = nullptr;
	QScopedPointer<AboutDlgPresener> pAboutDlgPresenter;

    ISystemBackend*     m_backend             = nullptr;
    SystemTreeModel*    m_pSystemTreeModel    = nullptr;
    SystemTreeDelegate* m_pSystemTreeDelegate = nullptr;
};

#endif // CPRESENTER_H
