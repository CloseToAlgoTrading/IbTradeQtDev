/********************************************************************************
** Form generated from reading UI file 'pairtrading.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PAIRTRADING_H
#define UI_PAIRTRADING_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PairTrading
{
public:
    QHBoxLayout *horizontalLayout;
    QTabWidget *tabWidget;
    QWidget *tabMain;
    QVBoxLayout *verticalLayout_3;
    QSplitter *splitter_3;
    QFrame *frame_2;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox_2;
    QPushButton *pushButtonRemoveSelected;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *lineEdit;
    QLineEdit *lineEdit_2;
    QPushButton *pushButtonAddPair;
    QSplitter *splitter;
    QTableView *tableView;
    QGroupBox *groupBox;
    QFrame *frame;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget_2;
    QWidget *tab_2;
    QGridLayout *gridLayout_4;
    QVBoxLayout *verticalLayout_Tab;
    QWidget *tab_3;
    QPushButton *pushButtonBuyMkt;
    QLineEdit *lineEditOrderSymbol;
    QSpinBox *spinBoxQuantity;
    QRadioButton *radioButtonBuy;
    QRadioButton *radioButtonSell;
    QPushButton *pushButtonSellMkt;
    QSplitter *splitter_2;
    QGroupBox *groupBoxPrice;
    QHBoxLayout *horizontalLayout_3;
    QWidget *tab;
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QButtonGroup *buttonGroup;

    void setupUi(QWidget *PairTrading)
    {
        if (PairTrading->objectName().isEmpty())
            PairTrading->setObjectName("PairTrading");
        PairTrading->resize(1020, 773);
        horizontalLayout = new QHBoxLayout(PairTrading);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName("horizontalLayout");
        tabWidget = new QTabWidget(PairTrading);
        tabWidget->setObjectName("tabWidget");
        tabMain = new QWidget();
        tabMain->setObjectName("tabMain");
        verticalLayout_3 = new QVBoxLayout(tabMain);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName("verticalLayout_3");
        splitter_3 = new QSplitter(tabMain);
        splitter_3->setObjectName("splitter_3");
        splitter_3->setOrientation(Qt::Horizontal);
        frame_2 = new QFrame(splitter_3);
        frame_2->setObjectName("frame_2");
        frame_2->setMinimumSize(QSize(421, 0));
        frame_2->setMaximumSize(QSize(421, 16777215));
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(frame_2);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName("verticalLayout_2");
        groupBox_2 = new QGroupBox(frame_2);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setMinimumSize(QSize(0, 82));
        pushButtonRemoveSelected = new QPushButton(groupBox_2);
        pushButtonRemoveSelected->setObjectName("pushButtonRemoveSelected");
        pushButtonRemoveSelected->setGeometry(QRect(244, 50, 131, 23));
        layoutWidget = new QWidget(groupBox_2);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(20, 20, 355, 27));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        lineEdit = new QLineEdit(layoutWidget);
        lineEdit->setObjectName("lineEdit");

        horizontalLayout_2->addWidget(lineEdit);

        lineEdit_2 = new QLineEdit(layoutWidget);
        lineEdit_2->setObjectName("lineEdit_2");

        horizontalLayout_2->addWidget(lineEdit_2);

        pushButtonAddPair = new QPushButton(layoutWidget);
        pushButtonAddPair->setObjectName("pushButtonAddPair");

        horizontalLayout_2->addWidget(pushButtonAddPair);


        verticalLayout_2->addWidget(groupBox_2);

        splitter = new QSplitter(frame_2);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Vertical);
        tableView = new QTableView(splitter);
        tableView->setObjectName("tableView");
        tableView->setMinimumSize(QSize(200, 200));
        splitter->addWidget(tableView);
        groupBox = new QGroupBox(splitter);
        groupBox->setObjectName("groupBox");
        groupBox->setMinimumSize(QSize(395, 245));
        groupBox->setMaximumSize(QSize(395, 245));
        splitter->addWidget(groupBox);

        verticalLayout_2->addWidget(splitter);

        splitter_3->addWidget(frame_2);
        frame = new QFrame(splitter_3);
        frame->setObjectName("frame");
        frame->setMinimumSize(QSize(300, 631));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget_2 = new QTabWidget(frame);
        tabWidget_2->setObjectName("tabWidget_2");
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        gridLayout_4 = new QGridLayout(tab_2);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName("gridLayout_4");
        verticalLayout_Tab = new QVBoxLayout();
        verticalLayout_Tab->setSpacing(6);
        verticalLayout_Tab->setObjectName("verticalLayout_Tab");

        gridLayout_4->addLayout(verticalLayout_Tab, 0, 0, 1, 1);

        tabWidget_2->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        pushButtonBuyMkt = new QPushButton(tab_3);
        pushButtonBuyMkt->setObjectName("pushButtonBuyMkt");
        pushButtonBuyMkt->setGeometry(QRect(20, 70, 75, 23));
        lineEditOrderSymbol = new QLineEdit(tab_3);
        lineEditOrderSymbol->setObjectName("lineEditOrderSymbol");
        lineEditOrderSymbol->setGeometry(QRect(20, 30, 71, 20));
        spinBoxQuantity = new QSpinBox(tab_3);
        spinBoxQuantity->setObjectName("spinBoxQuantity");
        spinBoxQuantity->setGeometry(QRect(110, 30, 71, 22));
        spinBoxQuantity->setMinimum(1);
        spinBoxQuantity->setMaximum(1000);
        spinBoxQuantity->setValue(100);
        radioButtonBuy = new QRadioButton(tab_3);
        buttonGroup = new QButtonGroup(PairTrading);
        buttonGroup->setObjectName("buttonGroup");
        buttonGroup->addButton(radioButtonBuy);
        radioButtonBuy->setObjectName("radioButtonBuy");
        radioButtonBuy->setGeometry(QRect(200, 20, 82, 17));
        radioButtonSell = new QRadioButton(tab_3);
        buttonGroup->addButton(radioButtonSell);
        radioButtonSell->setObjectName("radioButtonSell");
        radioButtonSell->setGeometry(QRect(200, 40, 82, 17));
        pushButtonSellMkt = new QPushButton(tab_3);
        pushButtonSellMkt->setObjectName("pushButtonSellMkt");
        pushButtonSellMkt->setGeometry(QRect(100, 70, 75, 23));
        tabWidget_2->addTab(tab_3, QString());

        verticalLayout->addWidget(tabWidget_2);

        splitter_2 = new QSplitter(frame);
        splitter_2->setObjectName("splitter_2");
        splitter_2->setOrientation(Qt::Vertical);
        groupBoxPrice = new QGroupBox(splitter_2);
        groupBoxPrice->setObjectName("groupBoxPrice");
        horizontalLayout_3 = new QHBoxLayout(groupBoxPrice);
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        splitter_2->addWidget(groupBoxPrice);

        verticalLayout->addWidget(splitter_2);

        splitter_3->addWidget(frame);

        verticalLayout_3->addWidget(splitter_3);

        tabWidget->addTab(tabMain, QString());
        tab = new QWidget();
        tab->setObjectName("tab");
        gridLayout_2 = new QGridLayout(tab);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setSizeConstraint(QLayout::SetDefaultConstraint);

        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

        tabWidget->addTab(tab, QString());

        horizontalLayout->addWidget(tabWidget);


        retranslateUi(PairTrading);

        tabWidget->setCurrentIndex(0);
        tabWidget_2->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(PairTrading);
    } // setupUi

    void retranslateUi(QWidget *PairTrading)
    {
        PairTrading->setWindowTitle(QCoreApplication::translate("PairTrading", "PairTrading", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("PairTrading", "Add Pair", nullptr));
        pushButtonRemoveSelected->setText(QCoreApplication::translate("PairTrading", "Remove Selected", nullptr));
        pushButtonAddPair->setText(QCoreApplication::translate("PairTrading", "Add", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PairTrading", "GroupBox", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_2), QCoreApplication::translate("PairTrading", "Tab 1", nullptr));
        pushButtonBuyMkt->setText(QCoreApplication::translate("PairTrading", "Buy", nullptr));
        lineEditOrderSymbol->setText(QCoreApplication::translate("PairTrading", "SPY", nullptr));
        radioButtonBuy->setText(QCoreApplication::translate("PairTrading", "Buy", nullptr));
        radioButtonSell->setText(QCoreApplication::translate("PairTrading", "Sell", nullptr));
        pushButtonSellMkt->setText(QCoreApplication::translate("PairTrading", "Sell", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_3), QCoreApplication::translate("PairTrading", "Tab 2", nullptr));
        groupBoxPrice->setTitle(QCoreApplication::translate("PairTrading", "Price", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabMain), QCoreApplication::translate("PairTrading", "Tab 1", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("PairTrading", "Page", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PairTrading: public Ui_PairTrading {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PAIRTRADING_H
