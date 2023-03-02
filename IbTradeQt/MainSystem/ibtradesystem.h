#ifndef IBTRADESYSTEM_H
#define IBTRADESYSTEM_H

#include <QtWidgets/QMainWindow>
#include <QtConcurrent/QtConcurrentRun>
#include "ui_ibtradesystem.h"
#include <QLabel>
#include "CTreeViewCustomModel.h"

class IBTradeSystem : public QMainWindow
{
	Q_OBJECT

public:
    IBTradeSystem(QWidget *parent = nullptr);
	~IBTradeSystem();

	Ui::IBTradeSystemClass getUi(){ return ui; }


private:
    void createSettingsView();




private:
	Ui::IBTradeSystemClass ui;
	QLabel * m_pTimeLabel;
    CTreeViewCustomModel m_SettingsModel;
    CTreeViewCustomModel m_portfolioConfigModel;


private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
	void slotRecvConnectButtonState(bool isConnect);
    void slotEditSettingLogSettingsComlited();
    void slotClearLog();
    void slotShowLog();
    void slotshowSettings();

};

#endif // IBTRADESYSTEM_H
