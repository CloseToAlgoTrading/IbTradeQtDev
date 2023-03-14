#ifndef IBTRADESYSTEM_H
#define IBTRADESYSTEM_H

#include <QtWidgets/QMainWindow>
#include <QtConcurrent/QtConcurrentRun>
#include "ui_ibtradesystem.h"
#include <QLabel>
#include "csettinsmodeldata.h"
#include "PortfolioConfigModel.h"


class IBTradeSystem : public QMainWindow
{
	Q_OBJECT

public:
    IBTradeSystem(QWidget *parent = nullptr);
	~IBTradeSystem();

    Ui::IBTradeSystemClass getUi();


private:
    void createSettingsView();




private:
	Ui::IBTradeSystemClass ui;
	QLabel * m_pTimeLabel;
    CSettinsModelData m_SettingsModel;
    PortfolioConfigModel m_portfolioConfigModel;


private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
	void slotRecvConnectButtonState(bool isConnect);
    void slotEditSettingLogSettingsComlited();
    void slotClearLog();
    void slotShowLog();
    void slotshowSettings();

    void slotUpdateTreeView(const QModelIndex& index);

};

#endif // IBTRADESYSTEM_H
