#include "DBStorePresenter.h"
#include <QMetaType>


DBStorePresenter::DBStorePresenter(QObject *parent, CBrokerDataProvider & _refClient)
    : QObject(parent)
    , m_pDBStoreGui(new DBStoreGui())
    , m_pDBStorePM(new DBStorePM(parent, _refClient))
    , m_threadDBStorePM(new QThread)
{

}

DBStorePresenter::~DBStorePresenter()
{

}

void DBStorePresenter::init()
{
    m_pDBStorePM->moveToThread(m_threadDBStorePM);
    m_threadDBStorePM->start();
    m_threadDBStorePM->setObjectName("DBStorePM_Thread");

    //qRegisterMetaType<QVector<QString>>();

    QObject::connect(m_pDBStoreGui.data(), &DBStoreGui::signalStartDBStore, m_pDBStorePM.data(), &DBStorePM::slotStartDBStore, Qt::QueuedConnection);
    QObject::connect(m_pDBStoreGui.data(), &DBStoreGui::signalStopDBStore, m_pDBStorePM.data(), &DBStorePM::slotStopDBStore, Qt::QueuedConnection);
    QObject::connect(m_pDBStoreGui.data(), &DBStoreGui::signalGUIClosed, m_pDBStorePM.data(), &DBStorePM::slotOnGUIClosed, Qt::QueuedConnection);

    QObject::connect(this, &DBStorePresenter::signalResetSubscribtion,
                     m_pDBStorePM.data(), &DBStorePM::slotRestartSubscription, Qt::QueuedConnection);
}

void DBStorePresenter::showDlg()
{
    m_pDBStoreGui->show();
}
