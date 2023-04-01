#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cpresenter.h"
#include "ibtradesystemview.h"
#include "cmainmodel.h"
#include <QSharedPointer>
#include <QApplication>
#include "cbasicroot.h"

class CApplicationController
{
public:
    CApplicationController();
    ~CApplicationController();

    void setUpApplication(QApplication &app);



    void setPMainModel(CMainModel *newPMainModel);

private:

    CPresenter *pMainPresenter;
    CIBTradeSystemView *pMainView;
    CMainModel *pMainModel;

    CBasicRoot *m_pDataRoot;


};

#endif // CAPPLICATIONCONTROLLER_H
