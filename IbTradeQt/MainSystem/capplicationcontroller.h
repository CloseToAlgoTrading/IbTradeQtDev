#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cbasicroot.h"

#include "cpresenter.h"
#include "ibtradesystemview.h"
#include "cmainmodel.h"
#include <QSharedPointer>
#include <QApplication>
#include <QObject>

class CApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit CApplicationController(QObject *parent = nullptr);
    virtual ~CApplicationController();

    void setUpApplication(QApplication &app);

    void setPMainModel(CMainModel *newPMainModel);

private:
    void loadTreeFromFile(const QString& fileName);


public slots:
    void slotStoreModelTree();

private:

    CPresenter *pMainPresenter;
    CIBTradeSystemView *pMainView;
    CMainModel *pMainModel;

    CBasicRoot *m_pDataRoot;
};

#endif // CAPPLICATIONCONTROLLER_H
