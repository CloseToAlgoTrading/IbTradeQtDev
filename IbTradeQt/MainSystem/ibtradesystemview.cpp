﻿#include "ibtradesystemview.h"
#include <time.h>
#include <QStandardItemModel>
#include "GlobalDef.h"
#include <QTime>
#include <QSharedPointer>


CIBTradeSystemView::CIBTradeSystemView(QWidget *parent)
	: QMainWindow(parent)
    , m_pTimeLabel(new QLabel(this))
    , m_pConnectLabel(new QLabel(ui.statusBar))
    , m_ConnectIcon()
{
	ui.setupUi(this);


    /*** Beggin Create Context Menu **************/
    ui.test_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Account.png"), "Add New Account");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Portfolio.png"), "Add New Portfolio");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add New Strategy");
    //ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add New SelectionModel");
    QAction *act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add Selection Model");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add Aplha Model");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add Rebalance Model");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add Risk Model");
    ui.test_treeView->addAction(QIcon(":/IBTradeSystem/x_resources/Strategy.png"), "Add Execution Model");
    act = new QAction(this);
    act->setSeparator(true);
    ui.test_treeView->addAction(act);
    ui.test_treeView->addAction("Remove Selected Node");

    /*** End Create Context Menu **************/

    this->setWindowTitle(QString("IB Trade v. ") + QString(APP_VERSION));



    //m_pTimeLabel = new QLabel(this);
	ui.statusBar->addWidget(m_pTimeLabel);
    ui.statusBar->addPermanentWidget(m_pConnectLabel);

    m_pConnectLabel->setPixmap(QIcon::fromTheme("network-offline").pixmap(16));
    m_pConnectLabel->setToolTip("Disconnected");
    //m_pConnectLabel->setText("Disconnected");

//    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &CIBTradeSystemView::slotClearLog);
//    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &CIBTradeSystemView::slotShowLog);
//    QObject::connect(ui.actionSetting, &QAction::triggered, this, &CIBTradeSystemView::slotshowSettings);


//    QObject::connect(ui.test_treeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });
//    QObject::connect(ui.test_treeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });

//    QObject::connect(ui.settingsTreeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });
//    QObject::connect(ui.settingsTreeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });

}

CIBTradeSystemView::~CIBTradeSystemView()
{

}

Ui::IBTradeSystemClass CIBTradeSystemView::getUi()
{
    return ui;
}


QTreeView *CIBTradeSystemView::getSettingsTreeView()
{
    return ui.settingsTreeView;
}

QTreeView *CIBTradeSystemView::getPortfolioConfigTreeView()
{
    return ui.test_treeView;
}

void CIBTradeSystemView::mapSignals()
{
    QObject::connect(ui.actionClear_Log, &QAction::triggered, this, &CIBTradeSystemView::slotClearLog);
    QObject::connect(ui.actionShow_Log, &QAction::triggered, this, &CIBTradeSystemView::slotShowLog);
    QObject::connect(ui.actionSetting, &QAction::triggered, this, &CIBTradeSystemView::slotshowSettings);

    QObject::connect(ui.test_treeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });
    QObject::connect(ui.test_treeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.test_treeView->resizeColumnToContents(index.column()); });

    QObject::connect(ui.settingsTreeView, &QTreeView::expanded,  [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });
    QObject::connect(ui.settingsTreeView, &QTreeView::collapsed, [=](const QModelIndex& index) { ui.settingsTreeView->resizeColumnToContents(index.column()); });
}

void CIBTradeSystemView::slotOnTimeReceived(long time)
{
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch((time_t)(time));
    QString dateTimeString = dateTime.toString("dd-MM-yyyy hh:mm:ss");
    m_pTimeLabel->setText(dateTimeString);
}


void CIBTradeSystemView::slotOnLogMsgReceived(QString msg)
{
	ui.textEdit->append(msg);
}

void CIBTradeSystemView::slotRecvConnectButtonState(bool isConnect)
{
	if (isConnect)
	{
        ui.actionConnect->setText("Disconnect");
        ui.actionConnect->setIcon(QIcon::fromTheme("network-offline"));

        m_pConnectLabel->setPixmap(QIcon::fromTheme("network-idle").pixmap(16));
        m_pConnectLabel->setToolTip("Connected");

	}
	else
	{
        ui.actionConnect->setText("Connect");
        ui.actionConnect->setIcon(QIcon::fromTheme("network-idle"));

        m_pConnectLabel->setPixmap(QIcon::fromTheme("network-offline").pixmap(16));
        m_pConnectLabel->setToolTip("Disconnected");

    }
}

void CIBTradeSystemView::slotClearLog()
{
    ui.textEdit->clear();
}

void CIBTradeSystemView::slotShowLog()
{
    ui.dockWidget_Logging->show();
}

void CIBTradeSystemView::slotshowSettings()
{
    ui.dockWidget_Settings->show();
}

void CIBTradeSystemView::slotUpdateTreeView(const QModelIndex &index)
{
    ui.test_treeView->update(index);
    ui.test_treeView->expand(index);
}

void CIBTradeSystemView::slotUpdateTreeViewAll()
{
    ui.test_treeView->update();
}


