#ifndef DBSTOREPM_H
#define DBSTOREPM_H

// #include "./Common/cprocessingbase.h"
// #include "DBConnector.h"
// #include "ctickbytickalllast.h"
// #include "dbstoremodel.h"

// Q_DECLARE_LOGGING_CATEGORY(DBStorePMLog);


// typedef QVector<QString> DBStoreTickerArrayType;

// class DBStorePM :/* public QObject,*/ public CProcessingBase
// {
//     Q_OBJECT

// public:
//     explicit DBStorePM(QObject *parent, CBrokerDataProvider & _refClient);
//     ~DBStorePM() override;

//     void initStrategy(const QString & s1);

// private:
//     qint32 Ticker_id;
//     DBConnector m_dbc;
//     QThread* m_threadDBConnector;
//     DBStoreTickerArrayType m_data;

//     void sendRequestData(const DBStoreTickerArrayType &_data);

// public slots:
//     void slotStartDBStore(const DBStoreTickerArrayType & _data);
//     void slotStopDBStore();
//     void slotOnRemoveSymbol(const QString & s1);
//     void slotOnGUIClosed();
//     void slotRestartSubscription();

// signals:
//     void signalInsertNewRealTimeBarItem(const CrealtimeBar& _item, const QString& _symbol);
//     void signalInsertNewTickByTickLastItem(const CTickByTickAllLast& _item, const QString& _symbol);

//     void signalRestartSubscription();

// public:
//     void recvRealtimeBar(void* pContext, tEReqType _reqType) override;
//     void recvTickByTickAllLastData(void* pContext, tEReqType _reqType) override;

//     void recvRestartSubscription() override;



// };

// Q_DECLARE_METATYPE(DBStoreTickerArrayType);

#endif // AUTODELTAALIGNMEPM_H
