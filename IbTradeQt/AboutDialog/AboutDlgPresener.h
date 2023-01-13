#ifndef ABOUTDLGPRESENER_H
#define ABOUTDLGPRESENER_H

#include <QObject>
#include <QScopedPointer>
#include "aboutdialog.h"

class AboutDlgPresener : public QObject
{
	Q_OBJECT

public:
	AboutDlgPresener(QObject *parent);
	~AboutDlgPresener();

	void showDlg();

private:
	QScopedPointer<AboutDialog> aboutDlg;
};

#endif // ABOUTDLGPRESENER_H
