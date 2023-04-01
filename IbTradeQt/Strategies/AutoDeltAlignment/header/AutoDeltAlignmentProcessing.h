#ifndef AUTODELTAALIGNMEPM_H
#define AUTODELTAALIGNMEPM_H

#include "./Common/cprocessingbase.h"
#include "cdeltaobject.h"
#include "autodeltatypes.h"
#include <QTimer>
#include <QSharedPointer>
#include "CModelInputData.h"
#include <QNetworkAccessManager>
#include <QSemaphore>
#include <QNetworkRequest>
#include "cbasicstrategy.h"

Q_DECLARE_LOGGING_CATEGORY(autoDeltaAligPMLog);



class autoDeltaAligPM :/* public QObject,*/ public CProcessingBase, public CBasicStrategy
{
    Q_OBJECT

public:
    explicit autoDeltaAligPM(QObject *parent, CBrokerDataProvider & _refClient);
    explicit autoDeltaAligPM(QObject *parent);
    ~autoDeltaAligPM() override;

    void initStrategy(const QString & s1);

    virtual bool start() override;
    virtual bool stop() override;

private:
    qint32 Ticker_id;
    CDeltaObject m_delta;
    QString m_TicketName;
    qreal m_commSum;
    qreal m_rpnlSum;
    qint32 m_deltaThresold;
    bool m_IsOrderBusy;

    CBrokerDataProvider m_DataProvider;

    tAutoDeltaOptDataType m_GuiInfo;
    //QSharedPointer<QTimer> m_pTimer;

    CModelInputData m_modelInput;
    QNetworkAccessManager m_networkManager;

    const QTime startWorkingTime;
    const QTime endWorkingTime;

    QSemaphore m_sem;
    QNetworkRequest request;

private:
    void startStrategy(const tAutoDeltaOptDataType & _opt, const qint32 &_delta);

public slots:
    void slotStartNewDeltaHedge(const tAutoDeltaOptDataType & _opt, const qint32 &_delta);
    void slotOnStopPressed();

    void slotOnGUIClosed();
    void slotMessageHandler(void* pContext, tEReqType _reqType) ;
    void slotRecvOptionTickComputation(const COptionTickComputation & obj);
    void slotEndRecvPosition();

    void slotOrderCommission(const qreal _commiss, const qreal _rpnl);

    void slotRestartSubscription();

    void slotTmpSendOrderBuy();
    void slotTmpSendOrderSell();

    void onManagerFinished(QNetworkReply *reply);

    //void slotTimerTriggered();

    void slotPostRequest();

signals:
    void signalOnRealTimeTickData(const IBDataTypes::CMyTickPrice & _pT, const QString _s2);
    void signalUpdateOptionDeltaGUI(const qreal _val);
    void signalUpdateBasisDeltaGUI(const qreal _val);

    void signalUpdateSumDeltaGUI(const qreal _val);
    void signalUpdateCommissionGUI(const qreal _val);
    void signalUpdateRPNLGUI(double _val);

    void signalPostRequest();


public:
    void callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol) override;
    void calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol) override;
    void callback_recvPositionEnd() override;

    void recvRealtimeBar(void* pContext, tEReqType _reqType) override;

    void TestFunc();


};

#endif // AUTODELTAALIGNMEPM_H
