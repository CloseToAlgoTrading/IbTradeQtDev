#ifndef IBTRADESYSTEMVIEW_H
#define IBTRADESYSTEMVIEW_H

#include <QtWidgets/QMainWindow>
#include <QtConcurrent/QtConcurrentRun>
#include "ciconhandler.h"
#include "ui_ibtradesystemview.h"
#include <QLabel>
#include <QIcon>

class CIBTradeSystemView : public QMainWindow
{
	Q_OBJECT

public:
    CIBTradeSystemView(QWidget *parent = nullptr);
    ~CIBTradeSystemView();

    Ui::IBTradeSystemClass getUi();

    QTreeView * getSettingsTreeView();
    QTreeView * getPortfolioConfigTreeView();

    void mapSignals();


private:
    Ui::IBTradeSystemClass ui;
    QLabel * m_pTimeLabel;
    QLabel * m_pConnectLabel;
    CIconHandler m_ih;


private slots:
	void slotOnTimeReceived(long time);
	void slotOnLogMsgReceived(QString msg);
	void slotRecvConnectButtonState(bool isConnect);
    void slotClearLog();
    void slotShowLog();
    void slotshowSettings();

    void slotUpdateTreeView(const QModelIndex& index);
public:
    void slotUpdateTreeViewAll();

};

#endif // IBTRADESYSTEMVIEW_H
