#include "ibtradesystem.h"
#include <time.h>
#include <QStandardItemModel>
#include "NHelper.h"
#include "MyLogger.h"
#include "GlobalDef.h"
#include <QTime>

IBTradeSystem::IBTradeSystem(QWidget *parent)
	: QMainWindow(parent)
    , m_settModel(parent)
{
	ui.setupUi(this);
	


    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));


	m_pTimeLabel = new QLabel(this);
	ui.statusBar->addWidget(m_pTimeLabel);

    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditLogSettingsCompleted, this, &IBTradeSystem::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &IBTradeSystem::slotClearLog);

    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(&m_settModel, &CStandartItemSettings::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

}

IBTradeSystem::~IBTradeSystem()
{

}


void IBTradeSystem::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(LOG_LEVEL_COUNT);
    CStandartItemSettings::updateLoggerSettingsArray(_mask, levelArr);
    m_settModel.setLoggerSettings(levelArr);


    m_settModel.setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    ui.tableViewSettings->setModel(m_settModel.getModel());
    QObject::connect(ui.tableViewSettings->model(), &QStandardItemModel::dataChanged,
                     &m_settModel, &CStandartItemSettings::dataChangeCallback, Qt::AutoConnection);

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

