#include "AboutDlgPresener.h"

AboutDlgPresener::AboutDlgPresener(QObject *parent)
	: QObject(parent)
	, aboutDlg(new AboutDialog())
{

}

AboutDlgPresener::~AboutDlgPresener()
{
	
}

void AboutDlgPresener::showDlg()
{
	aboutDlg->show();
}