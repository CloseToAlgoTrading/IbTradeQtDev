#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QWidget>
#include "ui_aboutdialog.h"

class AboutDialog : public QWidget
{
	Q_OBJECT

public:
    AboutDialog(QWidget *parent = nullptr);
	~AboutDialog();

	Ui::AboutDialog* getUi(){ return &ui; }
	
private:
	Ui::AboutDialog ui;
};

#endif // ABOUTDIALOG_H
