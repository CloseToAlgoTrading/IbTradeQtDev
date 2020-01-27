#include "AutoDeltAlignmentGUI.h"
//#include "GraphHelper.h"
#include <QMessageBox>
#include "NHelper.h"
#include <QLCDNumber>
#include <QTimer>


Q_LOGGING_CATEGORY(autoDeltaAligGuiLog, "AutoDeltaAlig.GUI");

AutoDeltaAligGui::AutoDeltaAligGui(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    QObject::connect(ui.buttonStartStop, SIGNAL(clicked()), this, SLOT(slotOnClickStartStopButton()));
    QObject::connect(this, &AutoDeltaAligGui::signalOnShow, this, &AutoDeltaAligGui::slotOnShow, Qt::QueuedConnection);

    //QObject::connect(this, &AutoDeltaAligGui::signalUpdateDelta, ui.lcdNumber_optDelta, &QLCDNumber::display, Qt::QueuedConnection);

    //ui.lcdNumber_optDelta->setPalette(Qt::green);
    ui.lcdNumber_optDelta->setSegmentStyle(QLCDNumber::Flat);
    ui.lcdNumber_basisDelta->setSegmentStyle(QLCDNumber::Flat);
    ui.lcdNumber_SumDelta->setSegmentStyle(QLCDNumber::Flat);

    ui.dateEditExpire->setDisplayFormat("dd/MM/yyyy");

}

AutoDeltaAligGui::~AutoDeltaAligGui()
{

}

const Ui::AutoDeltaAligForm& AutoDeltaAligGui::getUI()
{
    return ui;
}


//************************************
// Method:    closeEvent
// FullName:  AutoDeltaAligGui::closeEvent
// Access:    private
// Returns:   void
// Qualifier:
// Parameter: QCloseEvent * event
//************************************
void AutoDeltaAligGui::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "autoDeltaAligGui", "Quit?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        //qDebug() << "Yes was clicked";
        qCDebug(autoDeltaAligGuiLog(), "Yes was clicked");
        onCloseDialog();
        event->accept();
    }
    else {
        //qDebug() << "Yes was *not* clicked";
        qCDebug(autoDeltaAligGuiLog(), "Yes was *not* clicked");
        event->ignore();
    }

}

void AutoDeltaAligGui::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    emit signalOnShow();
}

//************************************
// Method:    onCloseDialog
// FullName:  AutoDeltaAligGui::onCloseDialog
// Access:    private
// Returns:   void
// Qualifier:
//************************************
void AutoDeltaAligGui::onCloseDialog()
{
    emit signalGUIClosed();
}


//************************************
// Method:    slotOnClickStartStopButton
// FullName:  AutoDeltaAligGui::slotOnClickStartStopButton
// Access:    public slot
// Returns:   void
// Qualifier:
//************************************
void AutoDeltaAligGui::slotOnClickStartStopButton()
{

    static bool isStarted = false;

    if(false == isStarted)
    {
        QString textTicket = ui.lineEdit_Ticket->text();
        bool isConditionOk = false;


        //remove all spaces from left and right
        textTicket = textTicket.trimmed();
        if (false == textTicket.isEmpty())
        {
            textTicket = textTicket.toUpper();
            isConditionOk = true;
        }

        qint32 _delta = ui.spinBox_Delta->value();
        qreal _target = ui.spinBox_TargetPrice->value();
        QString _date = ui.dateEditExpire->date().toString("yyyyMMdd");

        bool _isCall = ui.checkBox_OptionType->checkState();
        bool _isBuy = ui.checkBox_isBuy->checkState();


        if (true == isConditionOk)
        {
            isStarted = !isStarted;
            ui.buttonStartStop->setText("Stop");


            NHelper::writeSomeData("DeltaHedge/Ticker", textTicket);
            NHelper::writeSomeData("DeltaHedge/Delta", _delta);
            NHelper::writeSomeData("DeltaHedge/ExpDate", _target);
            NHelper::writeSomeData("DeltaHedge/TargetPrice", _date);
            NHelper::writeSomeData("DeltaHedge/isCall", _isCall);
            NHelper::writeSomeData("DeltaHedge/isBuy", _isBuy);

            //send Ticker to
            tAutoDeltaOptDataType _opt{textTicket, _date, _target, _isCall, _isBuy};
            emit signalStartNewDeltaHedge(_opt, _delta);
        }

    }
    else {
        isStarted = !isStarted;
        ui.buttonStartStop->setText("Start");
        emit signalStopButtonPressed();
    }

}

void AutoDeltaAligGui::slotOnShow()
{
    QString _ticker = NHelper::readSomeData("DeltaHedge/Ticker", "SPY").toString();
    qint32 _delta = NHelper::readSomeData("DeltaHedge/Delta", 10).toInt();
    QString _sdate = NHelper::readSomeData("DeltaHedge/TargetPrice", "10191020").toString();
    QDate _Date = QDate::fromString(_sdate,"yyyyMMdd");
    qreal _target = NHelper::readSomeData("DeltaHedge/ExpDate", 45).toReal();
    bool _isCall = NHelper::readSomeData("DeltaHedge/isCall", true).toBool();
    bool _isBuy = NHelper::readSomeData("DeltaHedge/isBuy", true).toBool();


    ui.spinBox_Delta->setValue(_delta);
    ui.spinBox_TargetPrice->setValue(_target);
    ui.lineEdit_Ticket->setText(_ticker);
    ui.dateEditExpire->setDate(_Date);
    ui.checkBox_OptionType->setCheckState(_isCall ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui.checkBox_isBuy->setCheckState(_isBuy ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

}

void AutoDeltaAligGui::slotOnUpdateOptionDeltaGUI(const qreal _val)
{
   // ui.lcdNumber_optDelta->
    ui.lcdNumber_optDelta->display(_val);
}

void AutoDeltaAligGui::slotOnUpdateBasisDeltaGUI(const qreal _val)
{
    ui.lcdNumber_basisDelta->display(_val);
}

void AutoDeltaAligGui::slotOnUpdateSumDeltaGUI(const qreal _val)
{
    ui.lcdNumber_SumDelta->display(_val);
}

void AutoDeltaAligGui::slotOnUpdateCommissionGUI(const qreal _val)
{
    ui.lcdNumber_com->display(_val);
}


