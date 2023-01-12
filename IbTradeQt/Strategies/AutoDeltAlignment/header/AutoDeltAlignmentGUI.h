#ifndef AUTODELTAALIGNMEGUI_H
#define AUTODELTAALIGNMEGUI_H


#include <QWidget>
#include <QStandardItemModel>
#include <QList>
#include <QMutex>
#include "ui_autodeltaaligform.h"
#include "MyLogger.h"
#include "autodeltatypes.h"
//#include "ctickprice.h"
//#include "clineqchart.h"
//#include "pairtraderpm.h"
//#include "ccandlestickqchart.h"

Q_DECLARE_LOGGING_CATEGORY(autoDeltaAligGuiLog);

class AutoDeltaAligGui : public QWidget
{
    Q_OBJECT

public:
    explicit AutoDeltaAligGui(QWidget *parent = nullptr);
    ~AutoDeltaAligGui();

    //void MessageHandler(void* pContext, tEReqType _reqType);
    //void UnsubscribeHandler(){};

    const Ui::AutoDeltaAligForm &getUI();


public slots:
    void slotOnClickStartStopButton();
    void slotOnShow();
    void slotOnUpdateOptionDeltaGUI(const qreal _val);
    void slotOnUpdateBasisDeltaGUI(const qreal _val);
    void slotOnUpdateSumDeltaGUI(const qreal _val);
    void slotOnUpdateCommissionGUI(const qreal _val);

signals:
    void signalGUIClosed();
    void signalStartNewDeltaHedge(const tAutoDeltaOptDataType & _opt, const qint32 & _delta);
    void signalStopButtonPressed();
    void signalOnShow();
    void signalUpdateDelta(double _val);

private:
    void onCloseDialog();

protected:
    //reimplementation
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent *e) override;




private:
    Ui::AutoDeltaAligForm ui;

};

#endif // AUTODELTAALIGNMEGUI_H
