#ifndef DBSTOREGUI_H
#define DBSTOREGUI_H


#include <QWidget>
#include <QStandardItemModel>
#include <QList>
#include <QMutex>
#include "MyLogger.h"
#include "ui_dbstroreform.h"
#include "dbstoremodel.h"
#include "DBStoreProcessing.h"

#define FILENAME_TICKERS "ticker.lst"

Q_DECLARE_LOGGING_CATEGORY(DBStoreGuiLog);

class DBStoreGui : public QWidget
{
    Q_OBJECT

public:
    explicit DBStoreGui(QWidget *parent = nullptr);
    ~DBStoreGui();

    DbStoreModel tickerModel;
    //void MessageHandler(void* pContext, tEReqType _reqType);
    //void UnsubscribeHandler(){};

    void addNewTicket(const QString & _symbol);
    void removeTicker();


public slots:
    void slotOnClickStartStopButton();
    void slotOnClickAddButton();
    void slotOnClickRemoveButton();

signals:
    void signalGUIClosed();
    void signalStartDBStore(const DBStoreTickerArrayType & _data);
    void signalStopDBStore();

private:
    void onCloseDialog();


protected:
    //reimplementation
    virtual void closeEvent(QCloseEvent* event);




private:
    Ui::DBStoreForm ui;

};

#endif // DBSTOREGUI_H
