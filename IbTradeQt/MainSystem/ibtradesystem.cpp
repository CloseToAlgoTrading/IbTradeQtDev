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
    , m_SettingsModel(ui.settingsTreeView, parent)
    , m_portfolioConfigModel(ui.test_treeView, parent)
{
	ui.setupUi(this);
    m_SettingsModel.setTreeView(ui.settingsTreeView);
    m_portfolioConfigModel.setTreeView(ui.test_treeView);


    /*** Beggin Create Context Menu **************/
    ui.test_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Account.png"), "Add New Account", &m_portfolioConfigModel, &PortfolioConfigModel::slotOnClickAddAccount, Qt::QueuedConnection);
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Portfolio.png"), "Add New Portfolio", &m_portfolioConfigModel, &PortfolioConfigModel::slotOnClickAddPortfolio, Qt::QueuedConnection);
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add New Strategy", &m_portfolioConfigModel, &PortfolioConfigModel::slotOnClickAddStrategy, Qt::QueuedConnection);
    QAction *act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);
    ui.test_treeView->addAction("Remove Selected Node", &m_portfolioConfigModel, &PortfolioConfigModel::onClickRemoveNodeButton, Qt::QueuedConnection);
    /*** End Create Context Menu **************/


    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));



	m_pTimeLabel = new QLabel(this);
	ui.statusBar->addWidget(m_pTimeLabel);

    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &IBTradeSystem::slotClearLog);
    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &IBTradeSystem::slotShowLog);
    QObject::connect(ui.actionSetting, &QAction::triggered, this, &IBTradeSystem::slotshowSettings);

    QObject::connect(&m_SettingsModel, &CSettinsModelData::signalEditLogSettingsCompleted, this, &IBTradeSystem::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(&m_SettingsModel, &CSettinsModelData::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(&m_SettingsModel, &CSettinsModelData::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    QObject::connect(ui.test_treeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });
    QObject::connect(ui.test_treeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });



    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

    //ui.test_treeView->setHeaderHidden(true);
    ui.test_treeView->setModel(&m_portfolioConfigModel);


    QObject::connect(&m_portfolioConfigModel, SIGNAL(signalUpdateData(QModelIndex)), this, SLOT(slotUpdateTreeView(QModelIndex)));


}

IBTradeSystem::~IBTradeSystem()
{

}

Ui::IBTradeSystemClass IBTradeSystem::getUi()
{
    return ui;
}


void IBTradeSystem::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(S_LOG_LEVEL_COUNT);

    CSettinsModelData::updateLoggerSettingsArray(_mask, levelArr);
    //CSettinsModelData* pData = static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject());
//    pData->setLoggerSettings(levelArr);
//    pData->setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    m_SettingsModel.setLoggerSettings(levelArr);
    m_SettingsModel.setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    ui.settingsTreeView->setModel(&m_SettingsModel);
    ui.settingsTreeView->expandAll();
}


void IBTradeSystem::slotOnTimeReceived(long time)
{
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch((time_t)(time));
    QString dateTimeString = dateTime.toString("dd-MM-yyyy hh:mm:ss");
    m_pTimeLabel->setText(dateTimeString);
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
    //quint8 mask = CSettinsModelData::getMaskFromLoggerSettings(static_cast<CSettinsModelData*>(m_SettingsModel.getDataObject())->getLoggerSettings());
    quint8 mask = CSettinsModelData::getMaskFromLoggerSettings(m_SettingsModel.getLoggerSettings());
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

void IBTradeSystem::slotUpdateTreeView(const QModelIndex &index)
{

    //ui.test_treeView->update();

    ui.test_treeView->update(index);
    ui.test_treeView->expand(index);

    //ui.test_treeView->expandAll();
}


