#include "capplicationcontroller.h"
#include "MyLogger.h"
#include <QtWidgets/QApplication>
#include <QProcess>
#include <QLibraryInfo>
#include <QStringList>
#include <QStandardPaths>
#include<QIcon>

CApplicationController::CApplicationController():
    pMainPresenter(new CPresenter(nullptr))
  , pMainView(new CIBTradeSystemView)
  , m_pDataRoot(new CBasicRoot)
{
    this->pMainPresenter->addView(this->pMainView);

    pMainModel =new CMainModel(pMainPresenter, m_pDataRoot, nullptr);
    this->pMainPresenter->setPGuiModel(this->pMainModel);
}



CApplicationController::~CApplicationController()
{
    delete this->pMainView;
    delete this->pMainPresenter;
}

void CApplicationController::setUpApplication(QApplication &app)
{
    QFont font("Courier New", 8);
    font.setStyleHint(QFont::Monospace);
    QApplication::setFont(font);

    this->pMainPresenter->MapSignals();
    this->pMainView->mapSignals();

    auto icon = QIcon(":/IBTradeSystem/x_resources/app.png");
    app.setWindowIcon(icon);

    this->pMainView->show();


//    QFile file("Combinear.qss");
//    file.open(QFile::ReadOnly | QFile::Text);
//    QTextStream stream(&file);
//    QString styleSheet = stream.readAll();

//    a.setStyleSheet(styleSheet);

//    file.close();

}

void CApplicationController::setPMainModel(CMainModel *newPMainModel)
{
    pMainModel = newPMainModel;
}
