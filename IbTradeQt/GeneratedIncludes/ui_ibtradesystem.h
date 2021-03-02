/********************************************************************************
** Form generated from reading UI file 'ibtradesystem.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IBTRADESYSTEM_H
#define UI_IBTRADESYSTEM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IBTradeSystemClass
{
public:
    QAction *actionClear_Log;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_2;
    QSplitter *splitter_2;
    QFrame *frameControlPanel;
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter;
    QFrame *frame_5;
    QGridLayout *gridLayout_2;
    QFrame *frame_3;
    QVBoxLayout *verticalLayout;
    QPushButton *pushButton;
    QPushButton *pushButtonPairTrader;
    QPushButton *autoDeltaButton;
    QPushButton *pushButtonDBStore;
    QFrame *frame_4;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_2;
    QTableView *tableView;
    QTableView *tableViewSettings;
    QFrame *frameTextLog;
    QGridLayout *gridLayout;
    QTextEdit *textEdit;
    QMenuBar *menuBar;
    QMenu *menuclear_log;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *IBTradeSystemClass)
    {
        if (IBTradeSystemClass->objectName().isEmpty())
            IBTradeSystemClass->setObjectName(QString::fromUtf8("IBTradeSystemClass"));
        IBTradeSystemClass->resize(895, 488);
        actionClear_Log = new QAction(IBTradeSystemClass);
        actionClear_Log->setObjectName(QString::fromUtf8("actionClear_Log"));
        actionClear_Log->setCheckable(false);
        actionClear_Log->setChecked(false);
        centralWidget = new QWidget(IBTradeSystemClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        splitter_2 = new QSplitter(centralWidget);
        splitter_2->setObjectName(QString::fromUtf8("splitter_2"));
        splitter_2->setOrientation(Qt::Vertical);
        frameControlPanel = new QFrame(splitter_2);
        frameControlPanel->setObjectName(QString::fromUtf8("frameControlPanel"));
        frameControlPanel->setMinimumSize(QSize(0, 120));
        frameControlPanel->setFrameShape(QFrame::StyledPanel);
        frameControlPanel->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frameControlPanel);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(frameControlPanel);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setFrameShape(QFrame::Box);
        splitter->setOrientation(Qt::Horizontal);
        frame_5 = new QFrame(splitter);
        frame_5->setObjectName(QString::fromUtf8("frame_5"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame_5->sizePolicy().hasHeightForWidth());
        frame_5->setSizePolicy(sizePolicy);
        frame_5->setMinimumSize(QSize(120, 0));
        frame_5->setMaximumSize(QSize(120, 16777215));
        frame_5->setFrameShape(QFrame::StyledPanel);
        frame_5->setFrameShadow(QFrame::Raised);
        gridLayout_2 = new QGridLayout(frame_5);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);
        gridLayout_2->setContentsMargins(4, 4, 0, 0);
        frame_3 = new QFrame(frame_5);
        frame_3->setObjectName(QString::fromUtf8("frame_3"));
        frame_3->setFrameShape(QFrame::NoFrame);
        frame_3->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame_3);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        pushButton = new QPushButton(frame_3);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        verticalLayout->addWidget(pushButton);

        pushButtonPairTrader = new QPushButton(frame_3);
        pushButtonPairTrader->setObjectName(QString::fromUtf8("pushButtonPairTrader"));

        verticalLayout->addWidget(pushButtonPairTrader);

        autoDeltaButton = new QPushButton(frame_3);
        autoDeltaButton->setObjectName(QString::fromUtf8("autoDeltaButton"));

        verticalLayout->addWidget(autoDeltaButton);

        pushButtonDBStore = new QPushButton(frame_3);
        pushButtonDBStore->setObjectName(QString::fromUtf8("pushButtonDBStore"));

        verticalLayout->addWidget(pushButtonDBStore);


        gridLayout_2->addWidget(frame_3, 0, 0, 1, 1, Qt::AlignTop);

        splitter->addWidget(frame_5);
        frame_4 = new QFrame(splitter);
        frame_4->setObjectName(QString::fromUtf8("frame_4"));
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Raised);
        verticalLayout_3 = new QVBoxLayout(frame_4);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        tableView = new QTableView(frame_4);
        tableView->setObjectName(QString::fromUtf8("tableView"));

        horizontalLayout_2->addWidget(tableView);

        tableViewSettings = new QTableView(frame_4);
        tableViewSettings->setObjectName(QString::fromUtf8("tableViewSettings"));
        tableViewSettings->setMinimumSize(QSize(0, 0));
        tableViewSettings->setMaximumSize(QSize(350, 16777215));
        tableViewSettings->setSelectionMode(QAbstractItemView::SingleSelection);
        tableViewSettings->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableViewSettings->verticalHeader()->setVisible(false);

        horizontalLayout_2->addWidget(tableViewSettings);


        verticalLayout_3->addLayout(horizontalLayout_2);

        splitter->addWidget(frame_4);

        horizontalLayout->addWidget(splitter);

        splitter_2->addWidget(frameControlPanel);
        frameTextLog = new QFrame(splitter_2);
        frameTextLog->setObjectName(QString::fromUtf8("frameTextLog"));
        frameTextLog->setMinimumSize(QSize(10, 20));
        frameTextLog->setBaseSize(QSize(0, 100));
        frameTextLog->setLayoutDirection(Qt::LeftToRight);
        frameTextLog->setFrameShape(QFrame::StyledPanel);
        frameTextLog->setFrameShadow(QFrame::Raised);
        gridLayout = new QGridLayout(frameTextLog);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        gridLayout->setContentsMargins(2, 0, 2, 2);
        textEdit = new QTextEdit(frameTextLog);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setFrameShape(QFrame::Box);

        gridLayout->addWidget(textEdit, 1, 0, 1, 1);

        splitter_2->addWidget(frameTextLog);

        verticalLayout_2->addWidget(splitter_2);

        IBTradeSystemClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(IBTradeSystemClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 895, 17));
        menuclear_log = new QMenu(menuBar);
        menuclear_log->setObjectName(QString::fromUtf8("menuclear_log"));
        IBTradeSystemClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(IBTradeSystemClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        IBTradeSystemClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(IBTradeSystemClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        IBTradeSystemClass->setStatusBar(statusBar);

        menuBar->addAction(menuclear_log->menuAction());
        menuclear_log->addAction(actionClear_Log);

        retranslateUi(IBTradeSystemClass);

        QMetaObject::connectSlotsByName(IBTradeSystemClass);
    } // setupUi

    void retranslateUi(QMainWindow *IBTradeSystemClass)
    {
        IBTradeSystemClass->setWindowTitle(QCoreApplication::translate("IBTradeSystemClass", "IBTradeSystem V1", nullptr));
        actionClear_Log->setText(QCoreApplication::translate("IBTradeSystemClass", "Clear Log", nullptr));
        pushButton->setText(QCoreApplication::translate("IBTradeSystemClass", "Connect", nullptr));
        pushButtonPairTrader->setText(QCoreApplication::translate("IBTradeSystemClass", "Pair Trader", nullptr));
        autoDeltaButton->setText(QCoreApplication::translate("IBTradeSystemClass", "Auto Delta", nullptr));
        pushButtonDBStore->setText(QCoreApplication::translate("IBTradeSystemClass", "DBStore", nullptr));
        menuclear_log->setTitle(QCoreApplication::translate("IBTradeSystemClass", "options", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IBTradeSystemClass: public Ui_IBTradeSystemClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IBTRADESYSTEM_H
