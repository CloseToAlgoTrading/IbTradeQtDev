#ifndef PAIRTRADINGPRESENTER_H
#define PAIRTRADINGPRESENTER_H

#include <QObject>
#include <QScopedPointer>
#include <QThread>
#include "PairTradingGui.h"
#include "pairtraderpm.h"
#include "./IBComm/cbrokerdataprovider.h"

class PairTradingPresenter : public QObject
{
	Q_OBJECT

public:
    explicit PairTradingPresenter(QObject *parent, CBrokerDataProvider & _refClient);
	~PairTradingPresenter();
	
    void init();

	void showDlg();

private:
	QScopedPointer<PairTradingGui> m_pPairTraderGui;
	QScopedPointer<PairTraderPM> m_pPairTraderPM;

    QThread* m_threadPairTaderPM;
	
};

#endif // PAIRTRADINGPRESENTER_H
