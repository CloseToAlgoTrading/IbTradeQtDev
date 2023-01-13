/********************************************************************************
** Form generated from reading UI file 'autodeltaaligform.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTODELTAALIGFORM_H
#define UI_AUTODELTAALIGFORM_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AutoDeltaAligForm
{
public:
    QFrame *frame;
    QLabel *label;
    QLineEdit *lineEdit_Ticket;
    QLabel *label_delta;
    QSpinBox *spinBox_Delta;
    QLabel *label_2;
    QLCDNumber *lcdNumber_pl;
    QLabel *label_3;
    QLineEdit *lineEdit_OrderStatus;
    QLabel *label_4;
    QLCDNumber *lcdNumber_com;
    QDateEdit *dateEditExpire;
    QDoubleSpinBox *spinBox_TargetPrice;
    QLabel *label_5;
    QLCDNumber *lcdNumber_optDelta;
    QLabel *label_6;
    QLCDNumber *lcdNumber_basisDelta;
    QLabel *label_7;
    QLCDNumber *lcdNumber_SumDelta;
    QCheckBox *checkBox_OptionType;
    QCheckBox *checkBox_isBuy;
    QPushButton *buttonStartStop;
    QPushButton *pushButton_Sell;
    QPushButton *pushButton_Buy;

    void setupUi(QWidget *AutoDeltaAligForm)
    {
        if (AutoDeltaAligForm->objectName().isEmpty())
            AutoDeltaAligForm->setObjectName("AutoDeltaAligForm");
        AutoDeltaAligForm->resize(400, 300);
        AutoDeltaAligForm->setMinimumSize(QSize(400, 300));
        AutoDeltaAligForm->setMaximumSize(QSize(400, 300));
        QFont font;
        font.setBold(false);
        AutoDeltaAligForm->setFont(font);
        frame = new QFrame(AutoDeltaAligForm);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(10, 10, 381, 231));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        label = new QLabel(frame);
        label->setObjectName("label");
        label->setGeometry(QRect(10, 10, 41, 21));
        lineEdit_Ticket = new QLineEdit(frame);
        lineEdit_Ticket->setObjectName("lineEdit_Ticket");
        lineEdit_Ticket->setGeometry(QRect(50, 10, 61, 21));
        label_delta = new QLabel(frame);
        label_delta->setObjectName("label_delta");
        label_delta->setGeometry(QRect(10, 40, 31, 16));
        spinBox_Delta = new QSpinBox(frame);
        spinBox_Delta->setObjectName("spinBox_Delta");
        spinBox_Delta->setGeometry(QRect(50, 40, 61, 22));
        spinBox_Delta->setMaximum(100);
        label_2 = new QLabel(frame);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(250, 10, 31, 16));
        lcdNumber_pl = new QLCDNumber(frame);
        lcdNumber_pl->setObjectName("lcdNumber_pl");
        lcdNumber_pl->setGeometry(QRect(290, 10, 81, 21));
        lcdNumber_pl->setDigitCount(5);
        label_3 = new QLabel(frame);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(10, 70, 31, 16));
        lineEdit_OrderStatus = new QLineEdit(frame);
        lineEdit_OrderStatus->setObjectName("lineEdit_OrderStatus");
        lineEdit_OrderStatus->setGeometry(QRect(50, 70, 61, 21));
        lineEdit_OrderStatus->setReadOnly(true);
        label_4 = new QLabel(frame);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(250, 40, 31, 16));
        lcdNumber_com = new QLCDNumber(frame);
        lcdNumber_com->setObjectName("lcdNumber_com");
        lcdNumber_com->setGeometry(QRect(290, 40, 81, 23));
        dateEditExpire = new QDateEdit(frame);
        dateEditExpire->setObjectName("dateEditExpire");
        dateEditExpire->setGeometry(QRect(120, 10, 121, 22));
        dateEditExpire->setLocale(QLocale(QLocale::English, QLocale::Germany));
        spinBox_TargetPrice = new QDoubleSpinBox(frame);
        spinBox_TargetPrice->setObjectName("spinBox_TargetPrice");
        spinBox_TargetPrice->setGeometry(QRect(120, 40, 71, 22));
        spinBox_TargetPrice->setDecimals(2);
        spinBox_TargetPrice->setMaximum(9999.989999999999782);
        label_5 = new QLabel(frame);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(220, 80, 61, 20));
        lcdNumber_optDelta = new QLCDNumber(frame);
        lcdNumber_optDelta->setObjectName("lcdNumber_optDelta");
        lcdNumber_optDelta->setGeometry(QRect(290, 80, 81, 23));
        QFont font1;
        font1.setBold(true);
        lcdNumber_optDelta->setFont(font1);
        lcdNumber_optDelta->setFrameShape(QFrame::StyledPanel);
        lcdNumber_optDelta->setLineWidth(1);
        lcdNumber_optDelta->setSmallDecimalPoint(false);
        lcdNumber_optDelta->setDigitCount(6);
        label_6 = new QLabel(frame);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(220, 110, 61, 16));
        lcdNumber_basisDelta = new QLCDNumber(frame);
        lcdNumber_basisDelta->setObjectName("lcdNumber_basisDelta");
        lcdNumber_basisDelta->setGeometry(QRect(290, 110, 81, 23));
        lcdNumber_basisDelta->setFrameShape(QFrame::StyledPanel);
        lcdNumber_basisDelta->setFrameShadow(QFrame::Raised);
        label_7 = new QLabel(frame);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(220, 140, 61, 20));
        lcdNumber_SumDelta = new QLCDNumber(frame);
        lcdNumber_SumDelta->setObjectName("lcdNumber_SumDelta");
        lcdNumber_SumDelta->setGeometry(QRect(290, 140, 81, 23));
        lcdNumber_SumDelta->setFrameShape(QFrame::StyledPanel);
        checkBox_OptionType = new QCheckBox(frame);
        checkBox_OptionType->setObjectName("checkBox_OptionType");
        checkBox_OptionType->setGeometry(QRect(200, 40, 41, 19));
        checkBox_isBuy = new QCheckBox(frame);
        checkBox_isBuy->setObjectName("checkBox_isBuy");
        checkBox_isBuy->setGeometry(QRect(120, 70, 72, 19));
        buttonStartStop = new QPushButton(AutoDeltaAligForm);
        buttonStartStop->setObjectName("buttonStartStop");
        buttonStartStop->setGeometry(QRect(310, 260, 80, 21));
        pushButton_Sell = new QPushButton(AutoDeltaAligForm);
        pushButton_Sell->setObjectName("pushButton_Sell");
        pushButton_Sell->setGeometry(QRect(20, 260, 80, 21));
        pushButton_Buy = new QPushButton(AutoDeltaAligForm);
        pushButton_Buy->setObjectName("pushButton_Buy");
        pushButton_Buy->setGeometry(QRect(120, 260, 80, 21));

        retranslateUi(AutoDeltaAligForm);

        QMetaObject::connectSlotsByName(AutoDeltaAligForm);
    } // setupUi

    void retranslateUi(QWidget *AutoDeltaAligForm)
    {
        AutoDeltaAligForm->setWindowTitle(QCoreApplication::translate("AutoDeltaAligForm", "AutoDeltaHedge", nullptr));
        label->setText(QCoreApplication::translate("AutoDeltaAligForm", "Ticker:", nullptr));
        lineEdit_Ticket->setText(QCoreApplication::translate("AutoDeltaAligForm", "SPY", nullptr));
        label_delta->setText(QCoreApplication::translate("AutoDeltaAligForm", "Delta:", nullptr));
        label_2->setText(QCoreApplication::translate("AutoDeltaAligForm", "P/L:", nullptr));
        label_3->setText(QCoreApplication::translate("AutoDeltaAligForm", "Order:", nullptr));
        label_4->setText(QCoreApplication::translate("AutoDeltaAligForm", "Com:", nullptr));
        dateEditExpire->setDisplayFormat(QCoreApplication::translate("AutoDeltaAligForm", "dd.MM.yyyy", nullptr));
        label_5->setText(QCoreApplication::translate("AutoDeltaAligForm", "Opt. delta:", nullptr));
        label_6->setText(QCoreApplication::translate("AutoDeltaAligForm", "Basis delta:", nullptr));
        label_7->setText(QCoreApplication::translate("AutoDeltaAligForm", "Delta sum:", nullptr));
        checkBox_OptionType->setText(QCoreApplication::translate("AutoDeltaAligForm", "Call", nullptr));
        checkBox_isBuy->setText(QCoreApplication::translate("AutoDeltaAligForm", "Buy", nullptr));
        buttonStartStop->setText(QCoreApplication::translate("AutoDeltaAligForm", "Start", nullptr));
        pushButton_Sell->setText(QCoreApplication::translate("AutoDeltaAligForm", "Sell", nullptr));
        pushButton_Buy->setText(QCoreApplication::translate("AutoDeltaAligForm", "Buy", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoDeltaAligForm: public Ui_AutoDeltaAligForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTODELTAALIGFORM_H
