#include "ibtradesystem.h"
#include <time.h>
#include <QStandardItemModel>
#include "NHelper.h"
#include "MyLogger.h"
#include "GlobalDef.h"
#include <QTime>
#include <QSharedPointer>

IBTradeSystem::IBTradeSystem(QWidget *parent)
	: QMainWindow(parent)
    , myModel(parent)
    , m_settModel(parent)
    , m_portfolioConfigModel(parent)
{
	ui.setupUi(this);
	


    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));



	m_pTimeLabel = new QLabel(this);
	ui.statusBar->addWidget(m_pTimeLabel);

    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditLogSettingsCompleted, this, &IBTradeSystem::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &IBTradeSystem::slotClearLog);
    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &IBTradeSystem::slotShowLog);
    QObject::connect(ui.actionSetting, &QAction::triggered, this, &IBTradeSystem::slotshowSettings);

    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

    //ui.test_treeView->setModel(&m_portfolioConfigModel);

}

IBTradeSystem::~IBTradeSystem()
{

}


void IBTradeSystem::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(LOG_LEVEL_COUNT);
    CStandartItemSettings::updateLoggerSettingsArray(_mask, levelArr);
//    m_settModel.setLoggerSettings(levelArr);


//    m_settModel.setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    ui.settingsTreeView->setModel(&myModel);

    QObject::connect(ui.settingsTreeView->model(), &QStandardItemModel::dataChanged,
                     &m_settModel, &CStandartItemSettings::dataChangeCallback, Qt::AutoConnection);
    ui.settingsTreeView->expandAll();

//    ui.test_treeView->setModel(&myModel);
//    ui.test_treeView->expandAll();
//    QObject::connect(ui.test_treeView->model(), &QStandardItemModel::dataChanged,
//                     &m_settModel, &CStandartItemSettings::dataChangeCallback, Qt::AutoConnection);
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
    quint8 mask = CStandartItemSettings::getMaskFromLoggerSettings(m_settModel.getLoggerSettings());
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

