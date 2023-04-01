#ifndef DBSTOREPRESENTER_H
#define DBSTOREPRESENTER_H

#include <QObject>
#include <QScopedPointer>
#include <QThread>
#include "DBStoreGUI.h"
#include "DBStoreProcessing.h"
#include "./IBComm/cbrokerdataprovider.h"

class DBStorePresenter : public QObject
{
    Q_OBJECT

public:
    explicit DBStorePresenter(QObject *parent, CBrokerDataProvider * _refClient);
    ~DBStorePresenter();

    void init();

    void showDlg();

signals:
    void signalResetSubscribtion();
private:
    QScopedPointer<DBStoreGui> m_pDBStoreGui;
    QScopedPointer<DBStorePM> m_pDBStorePM;

    QThread* m_threadDBStorePM;

};

#endif // DBSTOREPRESENTER_H
