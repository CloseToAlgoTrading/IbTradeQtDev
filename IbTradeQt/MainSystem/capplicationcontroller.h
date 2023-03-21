#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cpresenter.h"
#include "ibtradesystemview.h"
#include "cmainmodel.h"
#include <QSharedPointer>
#include <QApplication>

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


};

#endif // CAPPLICATIONCONTROLLER_H
