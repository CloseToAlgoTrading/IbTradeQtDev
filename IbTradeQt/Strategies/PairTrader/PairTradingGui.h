#ifndef PAIRTRADING_H
#define PAIRTRADING_H


// #include <QWidget>
// #include <QStandardItemModel>
// #include <QList>
// #include <QMutex>
// #include "ui_pairtrading.h"
// //#include "MyLogger.h"
// #include "ctickprice.h"
// #include "clineqchart.h"
// #include "pairtraderpm.h"
// #include "ccandlestickqchart.h"

// Q_DECLARE_LOGGING_CATEGORY(pairTraderGuiLog);

// class PairTradingGui : public QWidget
// {
// 	Q_OBJECT

// 	enum ColumIndexType
// 	{
// 		LAST_LEFT_INDEX = 0,
// 		BID_LEFT_INDEX,
// 		ASK_LEFT_INDEX,
// 		LEFT_INDEX,
// 		RIGHT_INDEX,
// 		BID_RIGHT_INDEX,
// 		ASK_RIGHT_INDEX,
// 		LAST_RIGHT_INDEX,
//         DEFAULT_INDEX_NONE
// 	};


//     typedef QMultiMap<qint32, ColumIndexType> rowUpdateMap_t;

// public:
//     explicit PairTradingGui(QWidget *parent = nullptr);
// 	~PairTradingGui();

// 	//void MessageHandler(void* pContext, tEReqType _reqType);
// 	//void UnsubscribeHandler(){};

// public slots:
// 	void slotOnClickAddButton();
// 	void slotOnClickRemoveButton();

//     void slotOnRealTimeTickData(const IBDataTypes::CMyTickPrice & _pTickPrice, const QString & _symbol);

//     void slotOnFinishHistoricalData(const QList<CHistoricalData> & _pT, const QString _s1);

//     //click buy/sell button
//     void slotOnClickedButtonBuyMkt();
//     void slotOnClickedButtonSellMkt();

//     //TableView Events
//     void slotOnClickedTableView(const QModelIndex &index);
//     void slotOnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

// signals:
// 	void signalAddNewPair(QString s1, QString s2);
//     void signalRemoveSimbol(const QString & s1);

//     void signalRequestHistory(const QString & s1);

    
//     //click buy/sell button
//     void signalRequestSendBuyMkt(const QString& _symbol, const quint32 _quantity);
//     void signalRequestSendSellMkt(const QString& _symbol, const quint32 _quantity);
    
//     void signalGUIClosed();
// private:
// 	bool isParePresents(const QString & s1, const QString & s2);
//     bool isSymbolCanBeRemoved(const QString & s1);
//     bool isSymbolFoundInTable(const QString & _s1, rowUpdateMap_t & _refMap);

//     void sendCancelRequestToPM(const QString _symbol);

//     void sendGetHistoryRequestToPM(const QString _symbol);

//     void sendGetHistoryForPair();

//     void onCloseDialog();

//     //helper
//     qint32 getSelectedRowFromTableView();

// protected:
//     //reimplementation
//     virtual void closeEvent(QCloseEvent* event);




// private:
// 	Ui::PairTrading ui;

// 	QStandardItemModel modelTable;

// 	//QList<QStandardItem*> *m_StandartItemList;

// 	//IBComClient& mClient;

// 	//MyLogger& m_pLog;

//     CLineQChart chartPair;
//     CLineQChart chartSpread;

//     CCandleStickQChart candleChart;



//     qint32 m_activeTableIndex;

//     QMutex mutex;

// };

#endif // PAIRTRADING_H
