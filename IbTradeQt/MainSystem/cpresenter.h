#ifndef CPRESENTER_H
#define CPRESENTER_H


#include <QObject>
#include "ibtradesystemview.h"
#include "IBworker.h"
#include "cbrokerdataprovider.h"
#include "AlphaModGetTime.h"
//#include "AutoDeltAlignmentPresenter.h"
//#include "DBStorePresenter.h"
#include <QScopedPointer>
#include "MyLogger.h"

#include "AboutDlgPresener.h"
//#include "PairTradingPresenter.h"


//#include "DBConnector.h"

class CMainModel;

class CPresenter : public QObject, public Observer::CSubscriber
{
	Q_OBJECT

public:
	CPresenter(QObject *parent);
	~CPresenter();

    void addView(CIBTradeSystemView * mw);

	void MessageHandler(void* pContext, tEReqType _reqType);
	void UnsubscribeHandler();

	void MapSignals();

	void myPrintf(const char* format, ...);


	MyLogger& m_pLog;

    CIBTradeSystemView *getPIbtsView() const;


    CMainModel *getPGuiModel() const;
    void setPGuiModel(CMainModel *newPGuiModel);

    QSharedPointer<CBrokerDataProvider> getDataProvider() const;

signals:
	void signalTimeReceived(long time);
	void signalClickConnect(bool isConnect);

private slots:
	void onClickMyButton();
	void onClickPairTraderButton();
    void onClickAutoDeltaButton();
    void onClickDBStoreButton();

private:

    //CBrokerDataProvider m_DataProvider;
    QSharedPointer<CBrokerDataProvider> m_pDataProvider;


    CIBTradeSystemView * pIbtsView;
    CMainModel * pGuiModel;
	
	QThread* threadIBClient;
	IBWorker::Worker* workerIBClient;

	QThread* threadAlfaTime;
	AlphaModGetTime* workerAlfaTime;


private:
	QScopedPointer<AboutDlgPresener> pAboutDlgPresenter;

    // QScopedPointer<PairTradingPresenter> pPairTradingPresenter;

 //    QScopedPointer<AutoDeltaAligPresenter> pAutoDeltaAligPresenter;

 //    QScopedPointer<DBStorePresenter> pDBStorePresenter;

};

#endif // CPRESENTER_H
