#ifndef IBTRADESYSTEM_H
#define IBTRADESYSTEM_H

#include <QtWidgets/QMainWindow>
#include <QtConcurrent/QtConcurrentRun>
#include "ui_ibtradesystem.h"
#include <QLabel>
//#include "settingsmodel.h"
#include "cstandartitemsettings.h"

class IBTradeSystem : public QMainWindow
{
	Q_OBJECT

public:
    IBTradeSystem(QWidget *parent = nullptr);
	~IBTradeSystem();

	Ui::IBTradeSystemClass getUi(){ return ui; }

public:
    //quint8 getMaskFromLoggerSettings(const bool _levels[]);
    //void updateLoggerSettingsArray(quint8 _mask, bool _levels[]);
    //CStandartItemSettings getSett() const;
    //void setSett(const CStandartItemSettings &value);

private:
    //void setupGraph(QCustomPlot *graphPlot);
    //void updateGraph(QCustomPlot *graphPlot, double key_x, double value);
    void createSettingsView();




private:
	Ui::IBTradeSystemClass ui;
	QLabel * m_pTimeLabel;
    //SettingsModel myModel;
    CStandartItemSettings m_settModel;

private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
	void slotRecvConnectButtonState(bool isConnect);
    void slotEditSettingLogSettingsComlited();
    void slotClearLog();

};

#endif // IBTRADESYSTEM_H
