#ifndef PAIRTRADERPM_H
#define PAIRTRADERPM_H

#include "./Common/cprocessingbase.h"

Q_DECLARE_LOGGING_CATEGORY(pairTraderPMLog);


class PairTraderPM :/* public QObject,*/ public CProcessingBase
{
	Q_OBJECT

public:
    explicit PairTraderPM(QObject *parent, CBrokerDataProvider & _refClient);
	~PairTraderPM();


    void initStrategy();
    void recvHistoryOfXIV(const QList<IBDataTypes::CHistoricalData> & _histMap);

private:
    qint32 XIV_id;

public slots:
	void slotOnAddNewPair(QString _s1, QString _s2);
    void slotOnRemoveSymbol(const QString & s1);
    void slotOnRequestHistory(const QString & s1);

    void slotOnGUIClosed();

    //test slots
    void slotOnRequestSendBuyMkt(const QString& _symbol, const quint32 _quantity);
    void slotOnRequestSendSellMkt(const QString& _symbol, const quint32 _quantity);

    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);

signals:

    void signalOnRealTimeTickData(const IBDataTypes::CMyTickPrice & _pT, const QString _s2);
    void signalOnFinishHistoricalData(const QList<CHistoricalData> & _pT, const QString _s1);


public:
    void callback_recvTickPrize(const IBDataTypes::CMyTickPrice _tickPrize, const QString& _symbol) override;
    void calllback_recvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol) override;



};

#endif // PAIRTRADERPM_H
