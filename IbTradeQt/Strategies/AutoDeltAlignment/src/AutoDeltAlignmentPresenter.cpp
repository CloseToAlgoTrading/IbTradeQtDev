#include "AutoDeltAlignmentPresenter.h"
#include <QMetaType>


AutoDeltaAligPresenter::AutoDeltaAligPresenter(QObject *parent, CBrokerDataProvider & _refClient)
    : QObject(parent)
    , m_pAutoDeltaAligGui(new AutoDeltaAligGui())
    , m_pAutoDeltaAligPM(new autoDeltaAligPM(parent, _refClient))
    , m_threadAlgoDeltaAligPM(new QThread)
{

}

AutoDeltaAligPresenter::~AutoDeltaAligPresenter()
{

}

void AutoDeltaAligPresenter::init()
{
    m_pAutoDeltaAligPM->moveToThread(m_threadAlgoDeltaAligPM);
    m_threadAlgoDeltaAligPM->start();
    m_threadAlgoDeltaAligPM->setObjectName("AutoDeltaPM_Thread");

    QObject::connect(m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::signalStartNewDeltaHedge, m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::slotStartNewDeltaHedge, Qt::QueuedConnection);
    QObject::connect(m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::signalGUIClosed, m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::slotOnGUIClosed, Qt::QueuedConnection);
    QObject::connect(m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::signalStopButtonPressed, m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::slotOnStopPressed, Qt::QueuedConnection);

    QObject::connect(m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::signalUpdateOptionDeltaGUI, m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::slotOnUpdateOptionDeltaGUI, Qt::QueuedConnection);
    QObject::connect(m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::signalUpdateBasisDeltaGUI, m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::slotOnUpdateBasisDeltaGUI, Qt::QueuedConnection);
    QObject::connect(m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::signalUpdateSumDeltaGUI, m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::slotOnUpdateSumDeltaGUI, Qt::QueuedConnection);

    QObject::connect(m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::signalUpdateCommissionGUI, m_pAutoDeltaAligGui.data(), &AutoDeltaAligGui::slotOnUpdateCommissionGUI, Qt::QueuedConnection);

    QObject::connect(m_pAutoDeltaAligPM.data(), SIGNAL(signalUpdateRPNLGUI(double)),
                     m_pAutoDeltaAligGui->getUI().lcdNumber_pl, SLOT(display(double)), Qt::QueuedConnection);
    //Todo: remove after order test
    QObject::connect(m_pAutoDeltaAligGui->getUI().pushButton_Buy, &QPushButton::clicked, m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::slotTmpSendOrderBuy, Qt::QueuedConnection);
    QObject::connect(m_pAutoDeltaAligGui->getUI().pushButton_Sell, &QPushButton::clicked, m_pAutoDeltaAligPM.data(), &autoDeltaAligPM::slotTmpSendOrderSell, Qt::QueuedConnection);

}

void AutoDeltaAligPresenter::showDlg()
{
    m_pAutoDeltaAligGui->show();
}

QSharedPointer<autoDeltaAligPM> AutoDeltaAligPresenter::getPM()
{
    return m_pAutoDeltaAligPM;
}
