// #include "DBStoreGUI.h"
// #include <QMessageBox>
// #include <QSortFilterProxyModel>
// #include <QAbstractItemModel>
// #include <QCloseEvent>


// Q_LOGGING_CATEGORY(DBStoreGuiLog, "DBStore.GUI");


// DBStoreGui::DBStoreGui(QWidget *parent)
//     : QWidget(parent)
//     , tickerModel()
// {
//     ui.setupUi(this);

//     tickerModel.readFromFile(FILENAME_TICKERS);

//     ui.tableViewTickers->setModel(&tickerModel);
//     ui.tableViewTickers->setSelectionMode( QAbstractItemView::SingleSelection );

//     QObject::connect(ui.pushButtonStart, SIGNAL(clicked()), this, SLOT(slotOnClickStartStopButton()));
//     QObject::connect(ui.pushButtonAdd, &QPushButton::clicked, this, &DBStoreGui::slotOnClickAddButton);
//     QObject::connect(ui.pushButtonRemove, &QPushButton::clicked, this, &DBStoreGui::slotOnClickRemoveButton);
// }

// DBStoreGui::~DBStoreGui()
// {

// }

// void DBStoreGui::addNewTicket(const QString &_symbol)
// {
//     qint32 pos = tickerModel.getModelData().length();
//     tickerModel.insertRows(pos, 1, QModelIndex());

//     QModelIndex index = tickerModel.index(pos, 0, QModelIndex());
//     tickerModel.setData(index, _symbol, Qt::EditRole);
// }

// void DBStoreGui::removeTicker()
// {
//     //TODO: multiselection
// //    QTableView *temp = ui.tableViewTickers;
// //    QAbstractProxyModel *proxy = static_cast<QAbstractProxyModel*>(temp->model());
// //    QItemSelectionModel *selectionModel = temp->selectionModel();

// //    QModelIndexList indexes = selectionModel->selectedRows();

// //    foreach (QModelIndex index, indexes) {
// //        int row = proxy->mapToSource(index).row();
// //        tickerModel.removeRows(row, 1, QModelIndex());
// //    }

//     QTableView *temp = ui.tableViewTickers;
//     //QSortFilterProxyModel *proxy = static_cast<QSortFilterProxyModel*>(temp->model());

//     QItemSelectionModel *selectionModel = ui.tableViewTickers->selectionModel();

//     QModelIndexList indexes = selectionModel->selectedRows();

//     foreach (QModelIndex index, indexes) {
//         int row = index.row();
//         tickerModel.removeRows(row, 1, QModelIndex());
//     }
// }


// //************************************
// // Method:    closeEvent
// // FullName:  DBStoreGui::closeEvent
// // Access:    private
// // Returns:   void
// // Qualifier:
// // Parameter: QCloseEvent * event
// //************************************
// void DBStoreGui::closeEvent(QCloseEvent* event)
// {
//     QMessageBox::StandardButton reply;
//     reply = QMessageBox::question(this, "DBStoreGui", "Quit?",
//         QMessageBox::Yes | QMessageBox::No);
//     if (reply == QMessageBox::Yes) {
//         //qDebug() << "Yes was clicked";
//         qCDebug(DBStoreGuiLog(), "Yes was clicked");
//         tickerModel.writeToFile(FILENAME_TICKERS);
//         onCloseDialog();
//         event->accept();
//     }
//     else {
//         //qDebug() << "Yes was *not* clicked";
//         qCDebug(DBStoreGuiLog(), "Yes was *not* clicked");
//         event->ignore();
//     }

// }

// //************************************
// // Method:    onCloseDialog
// // FullName:  DBStoreGui::onCloseDialog
// // Access:    private
// // Returns:   void
// // Qualifier:
// //************************************
// void DBStoreGui::onCloseDialog()
// {
//     emit signalGUIClosed();
// }


// //************************************
// // Method:    slotOnClickStartStopButton
// // FullName:  DBStoreGui::slotOnClickStartStopButton
// // Access:    public slot
// // Returns:   void
// // Qualifier:
// //************************************
// void DBStoreGui::slotOnClickStartStopButton()
// {
//     static bool isStarted = false;

//     if(false == isStarted)
//     {
//         if(tickerModel.getModelData().length() != 0)
//         {
//             isStarted = !isStarted;
//             ui.pushButtonStart->setText("Stop");
//             DBStoreTickerArrayType _data;
//             for (const auto &_Ticker: qAsConst(tickerModel.getModelData()))
//             {
//                 if(true == _Ticker.isChecked)
//                 {
//                     _data.push_back(_Ticker.symbol);
//                 }
//             }

//             emit signalStartDBStore(_data);
//         }

//     }
//     else {
//         isStarted = !isStarted;
//         ui.pushButtonStart->setText("Start");
//         emit signalStopDBStore();
//     }

// }

// void DBStoreGui::slotOnClickAddButton()
// {
//     QString textTicket = ui.lineEditSymbol->text().trimmed();

//     if (false == textTicket.isEmpty())
//     {
//         textTicket = textTicket.toUpper();
//             //check if pare presents in the table
//         bool isFound = false;

//         for (const auto &_Ticker: qAsConst(tickerModel.getModelData()))
//         {
//             if(_Ticker.symbol == textTicket)
//             {
//                 isFound = true;
//                 break;
//             }
//         }

//         if (false == isFound)
//         {
//             addNewTicket(textTicket);
//         }
//     }

// }

// void DBStoreGui::slotOnClickRemoveButton()
// {
//     removeTicker();
// }
