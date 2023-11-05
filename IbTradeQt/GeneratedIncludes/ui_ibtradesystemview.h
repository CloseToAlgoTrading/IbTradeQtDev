/********************************************************************************
** Form generated from reading UI file 'ibtradesystemview.ui'
**
** Created by: Qt User Interface Compiler version 6.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IBTRADESYSTEMVIEW_H
#define UI_IBTRADESYSTEMVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
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
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IBTradeSystemClass
{
public:
    QAction *actionClear_Log;
    QAction *actionShow_Log;
    QAction *actionSetting;
    QAction *actionAdd_Model;
    QAction *actionRemove_Model;
    QAction *actionLoad;
    QAction *actionSave;
    QAction *actionConnect;
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
    QTreeView *test_treeView;
    QMenuBar *menuBar;
    QMenu *menuclear_log;
    QMenu *menuView;
    QMenu *menuConfiguration;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QDockWidget *dockWidget_Logging;
    QWidget *dockWidgetContents_2;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_3;
    QTextEdit *textEdit;
    QDockWidget *dockWidget_Settings;
    QWidget *dockWidgetContents_3;
    QGridLayout *gridLayout_3;
    QHBoxLayout *horizontalLayout_4;
    QTreeView *settingsTreeView;

    void setupUi(QMainWindow *IBTradeSystemClass)
    {
        if (IBTradeSystemClass->objectName().isEmpty())
            IBTradeSystemClass->setObjectName("IBTradeSystemClass");
        IBTradeSystemClass->resize(895, 488);
        actionClear_Log = new QAction(IBTradeSystemClass);
        actionClear_Log->setObjectName("actionClear_Log");
        actionClear_Log->setCheckable(false);
        actionClear_Log->setChecked(false);
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("edit-clear")));
        actionClear_Log->setIcon(icon);
        actionShow_Log = new QAction(IBTradeSystemClass);
        actionShow_Log->setObjectName("actionShow_Log");
        actionShow_Log->setCheckable(false);
        actionShow_Log->setChecked(false);
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("text-x-generic")));
        actionShow_Log->setIcon(icon1);
        actionSetting = new QAction(IBTradeSystemClass);
        actionSetting->setObjectName("actionSetting");
        actionSetting->setCheckable(false);
        actionSetting->setChecked(false);
        QIcon icon2(QIcon::fromTheme(QString::fromUtf8("preferences-other")));
        actionSetting->setIcon(icon2);
        actionAdd_Model = new QAction(IBTradeSystemClass);
        actionAdd_Model->setObjectName("actionAdd_Model");
        actionRemove_Model = new QAction(IBTradeSystemClass);
        actionRemove_Model->setObjectName("actionRemove_Model");
        actionLoad = new QAction(IBTradeSystemClass);
        actionLoad->setObjectName("actionLoad");
        QIcon icon3(QIcon::fromTheme(QString::fromUtf8("document-open")));
        actionLoad->setIcon(icon3);
        actionSave = new QAction(IBTradeSystemClass);
        actionSave->setObjectName("actionSave");
        QIcon icon4(QIcon::fromTheme(QString::fromUtf8("document-save")));
        actionSave->setIcon(icon4);
        actionConnect = new QAction(IBTradeSystemClass);
        actionConnect->setObjectName("actionConnect");
        QIcon icon5(QIcon::fromTheme(QString::fromUtf8("network-idle")));
        actionConnect->setIcon(icon5);
        centralWidget = new QWidget(IBTradeSystemClass);
        centralWidget->setObjectName("centralWidget");
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        splitter_2 = new QSplitter(centralWidget);
        splitter_2->setObjectName("splitter_2");
        splitter_2->setOrientation(Qt::Vertical);
        frameControlPanel = new QFrame(splitter_2);
        frameControlPanel->setObjectName("frameControlPanel");
        frameControlPanel->setMinimumSize(QSize(0, 120));
        frameControlPanel->setFrameShape(QFrame::StyledPanel);
        frameControlPanel->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frameControlPanel);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(frameControlPanel);
        splitter->setObjectName("splitter");
        splitter->setFrameShape(QFrame::Box);
        splitter->setOrientation(Qt::Horizontal);
        frame_5 = new QFrame(splitter);
        frame_5->setObjectName("frame_5");
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
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setSizeConstraint(QLayout::SetDefaultConstraint);
        gridLayout_2->setContentsMargins(4, 4, 0, 0);
        frame_3 = new QFrame(frame_5);
        frame_3->setObjectName("frame_3");
        frame_3->setFrameShape(QFrame::NoFrame);
        frame_3->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame_3);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        pushButton = new QPushButton(frame_3);
        pushButton->setObjectName("pushButton");

        verticalLayout->addWidget(pushButton);

        pushButtonPairTrader = new QPushButton(frame_3);
        pushButtonPairTrader->setObjectName("pushButtonPairTrader");

        verticalLayout->addWidget(pushButtonPairTrader);

        autoDeltaButton = new QPushButton(frame_3);
        autoDeltaButton->setObjectName("autoDeltaButton");

        verticalLayout->addWidget(autoDeltaButton);

        pushButtonDBStore = new QPushButton(frame_3);
        pushButtonDBStore->setObjectName("pushButtonDBStore");

        verticalLayout->addWidget(pushButtonDBStore);


        gridLayout_2->addWidget(frame_3, 0, 0, 1, 1, Qt::AlignTop);

        splitter->addWidget(frame_5);
        frame_4 = new QFrame(splitter);
        frame_4->setObjectName("frame_4");
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Raised);
        verticalLayout_3 = new QVBoxLayout(frame_4);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        test_treeView = new QTreeView(frame_4);
        test_treeView->setObjectName("test_treeView");

        horizontalLayout_2->addWidget(test_treeView);


        verticalLayout_3->addLayout(horizontalLayout_2);

        splitter->addWidget(frame_4);

        horizontalLayout->addWidget(splitter);

        splitter_2->addWidget(frameControlPanel);

        verticalLayout_2->addWidget(splitter_2);

        IBTradeSystemClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(IBTradeSystemClass);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 895, 22));
        menuclear_log = new QMenu(menuBar);
        menuclear_log->setObjectName("menuclear_log");
        menuView = new QMenu(menuBar);
        menuView->setObjectName("menuView");
        menuConfiguration = new QMenu(menuBar);
        menuConfiguration->setObjectName("menuConfiguration");
        IBTradeSystemClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(IBTradeSystemClass);
        mainToolBar->setObjectName("mainToolBar");
        mainToolBar->setIconSize(QSize(16, 16));
        IBTradeSystemClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(IBTradeSystemClass);
        statusBar->setObjectName("statusBar");
        IBTradeSystemClass->setStatusBar(statusBar);
        dockWidget_Logging = new QDockWidget(IBTradeSystemClass);
        dockWidget_Logging->setObjectName("dockWidget_Logging");
        dockWidget_Logging->setMinimumSize(QSize(295, 212));
        dockWidget_Logging->setLayoutDirection(Qt::LeftToRight);
        dockWidget_Logging->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetVerticalTitleBar);
        dockWidget_Logging->setAllowedAreas(Qt::BottomDockWidgetArea);
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName("dockWidgetContents_2");
        gridLayout = new QGridLayout(dockWidgetContents_2);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName("gridLayout");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        textEdit = new QTextEdit(dockWidgetContents_2);
        textEdit->setObjectName("textEdit");
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(textEdit->sizePolicy().hasHeightForWidth());
        textEdit->setSizePolicy(sizePolicy1);
        textEdit->setFrameShape(QFrame::NoFrame);
        textEdit->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);

        horizontalLayout_3->addWidget(textEdit);


        gridLayout->addLayout(horizontalLayout_3, 0, 0, 1, 1);

        dockWidget_Logging->setWidget(dockWidgetContents_2);
        IBTradeSystemClass->addDockWidget(Qt::BottomDockWidgetArea, dockWidget_Logging);
        dockWidget_Settings = new QDockWidget(IBTradeSystemClass);
        dockWidget_Settings->setObjectName("dockWidget_Settings");
        dockWidget_Settings->setMaximumSize(QSize(350, 524287));
        dockWidget_Settings->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);
        dockWidget_Settings->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName("dockWidgetContents_3");
        dockWidgetContents_3->setEnabled(true);
        gridLayout_3 = new QGridLayout(dockWidgetContents_3);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName("gridLayout_3");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        settingsTreeView = new QTreeView(dockWidgetContents_3);
        settingsTreeView->setObjectName("settingsTreeView");

        horizontalLayout_4->addWidget(settingsTreeView);


        gridLayout_3->addLayout(horizontalLayout_4, 0, 0, 1, 1);

        dockWidget_Settings->setWidget(dockWidgetContents_3);
        IBTradeSystemClass->addDockWidget(Qt::RightDockWidgetArea, dockWidget_Settings);

        menuBar->addAction(menuclear_log->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuConfiguration->menuAction());
        menuclear_log->addAction(actionConnect);
        menuclear_log->addSeparator();
        menuclear_log->addAction(actionClear_Log);
        menuView->addAction(actionShow_Log);
        menuView->addAction(actionSetting);
        menuConfiguration->addAction(actionLoad);
        menuConfiguration->addAction(actionSave);
        mainToolBar->addAction(actionConnect);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionSetting);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionShow_Log);
        mainToolBar->addAction(actionClear_Log);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionLoad);
        mainToolBar->addAction(actionSave);

        retranslateUi(IBTradeSystemClass);

        QMetaObject::connectSlotsByName(IBTradeSystemClass);
    } // setupUi

    void retranslateUi(QMainWindow *IBTradeSystemClass)
    {
        IBTradeSystemClass->setWindowTitle(QCoreApplication::translate("IBTradeSystemClass", "IBTradeSystem V1", nullptr));
        actionClear_Log->setText(QCoreApplication::translate("IBTradeSystemClass", "Clear Log", nullptr));
        actionShow_Log->setText(QCoreApplication::translate("IBTradeSystemClass", "Log", nullptr));
        actionSetting->setText(QCoreApplication::translate("IBTradeSystemClass", "Setting", nullptr));
        actionAdd_Model->setText(QCoreApplication::translate("IBTradeSystemClass", "Add Model", nullptr));
        actionRemove_Model->setText(QCoreApplication::translate("IBTradeSystemClass", "Remove Model", nullptr));
        actionLoad->setText(QCoreApplication::translate("IBTradeSystemClass", "Load ...", nullptr));
        actionSave->setText(QCoreApplication::translate("IBTradeSystemClass", "Save ...", nullptr));
        actionConnect->setText(QCoreApplication::translate("IBTradeSystemClass", "Connect", nullptr));
        pushButton->setText(QCoreApplication::translate("IBTradeSystemClass", "Connect", nullptr));
        pushButtonPairTrader->setText(QCoreApplication::translate("IBTradeSystemClass", "Pair Trader", nullptr));
        autoDeltaButton->setText(QCoreApplication::translate("IBTradeSystemClass", "Auto Delta", nullptr));
        pushButtonDBStore->setText(QCoreApplication::translate("IBTradeSystemClass", "DBStore", nullptr));
        menuclear_log->setTitle(QCoreApplication::translate("IBTradeSystemClass", "options", nullptr));
        menuView->setTitle(QCoreApplication::translate("IBTradeSystemClass", "View", nullptr));
        menuConfiguration->setTitle(QCoreApplication::translate("IBTradeSystemClass", "Configuration", nullptr));
        dockWidget_Logging->setWindowTitle(QCoreApplication::translate("IBTradeSystemClass", "Logs", nullptr));
        dockWidget_Settings->setWindowTitle(QCoreApplication::translate("IBTradeSystemClass", "Settings", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IBTradeSystemClass: public Ui_IBTradeSystemClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IBTRADESYSTEMVIEW_H
