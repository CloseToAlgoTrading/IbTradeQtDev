#include "ibtradesystem.h"
#include <time.h>
#include <QStandardItemModel>
#include "NHelper.h"
#include "MyLogger.h"
#include "GlobalDef.h"
#include <QTime>
#include <QSharedPointer>
#include "csettinsmodeldata.h"
#include "PortfolioConfigModel.h"

IBTradeSystem::IBTradeSystem(QWidget *parent)
	: QMainWindow(parent)
    , m_SettingsModel(parent, new CSettinsModelData(parent))
    , m_portfolioConfigModel(parent, new PortfolioConfigModel(parent))
{
	ui.setupUi(this);

    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));



	m_pTimeLabel = new QLabel(this);
	ui.statusBar->addWidget(m_pTimeLabel);

    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &IBTradeSystem::slotClearLog);
    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &IBTradeSystem::slotShowLog);
    QObject::connect(ui.actionSetting, &QAction::triggered, this, &IBTradeSystem::slotshowSettings);

    QObject::connect(static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject()), &CSettinsModelData::signalEditLogSettingsCompleted, this, &IBTradeSystem::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject()), &CSettinsModelData::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject()), &CSettinsModelData::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

    //ui.test_treeView->setHeaderHidden(true);
    ui.test_treeView->setModel(&m_portfolioConfigModel);





}

IBTradeSystem::~IBTradeSystem()
{

}


void IBTradeSystem::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(S_LOG_LEVEL_COUNT);

    CSettinsModelData::updateLoggerSettingsArray(_mask, levelArr);
    CSettinsModelData* pData = static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject());
    pData->setLoggerSettings(levelArr);
    pData->setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    ui.settingsTreeView->setModel(&m_SettingsModel);
    ui.settingsTreeView->expandAll();
}


void IBTradeSystem::slotOnTimeReceived(long time)
{
    
    time_t t = (time_t)(time);
    struct tm * timeinfo = localtime(&t);

    QString s(asctime(timeinfo));
	s.remove(s.length() - 1, 1);
	m_pTimeLabel->setText(s);
}


void IBTradeSystem::slotOnLogMsgReceived(QString msg)
{
	ui.textEdit->append(msg);
}

void IBTradeSystem::slotRecvConnectButtonState(bool isConnect)
{
	if (isConnect)
	{
		ui.pushButton->setText("Disconnect");
	}
	else
	{
		ui.pushButton->setText("Connect");
    }
}

void IBTradeSystem::slotEditSettingLogSettingsComlited()
{
    quint8 mask = CSettinsModelData::getMaskFromLoggerSettings(static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject())->getLoggerSettings());
    NHelper::writeLoggerMask(mask);
    MyLogger::setDebugLevelMask(mask);
}

void IBTradeSystem::slotClearLog()
{
    ui.textEdit->clear();
}

void IBTradeSystem::slotShowLog()
{
    ui.dockWidget_Logging->show();
}

void IBTradeSystem::slotshowSettings()
{
    ui.dockWidget_Settings->show();
}

