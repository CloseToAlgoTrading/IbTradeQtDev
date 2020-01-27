#include "PairTradingGui.h"
#include "GraphHelper.h"
#include <QMessageBox>


Q_LOGGING_CATEGORY(pairTraderGuiLog, "pairTrader.GUI");

PairTradingGui::PairTradingGui(QWidget *parent)
	: QWidget(parent)
	, modelTable(new QStandardItemModel(2, 8, this))
    , chartPair(this)
    , chartSpread(this)
    , candleChart(this)
    , m_activeTableIndex(-1)
{
	ui.setupUi(this);

	//Configure graphs
	QString timeFormat("hh:mm:ss");


	//Set Table View
	modelTable.setHorizontalHeaderItem(LAST_LEFT_INDEX, new QStandardItem(QString("Last")));
	modelTable.setHorizontalHeaderItem(BID_LEFT_INDEX,  new QStandardItem(QString("Bid")));
	modelTable.setHorizontalHeaderItem(ASK_LEFT_INDEX,  new QStandardItem(QString("Ask")));
	modelTable.setHorizontalHeaderItem(LEFT_INDEX,      new QStandardItem(QString("Left")));
	modelTable.setHorizontalHeaderItem(RIGHT_INDEX,     new QStandardItem(QString("Right")));
	modelTable.setHorizontalHeaderItem(BID_RIGHT_INDEX, new QStandardItem(QString("Bid")));
	modelTable.setHorizontalHeaderItem(ASK_RIGHT_INDEX, new QStandardItem(QString("Ask")));
	modelTable.setHorizontalHeaderItem(LAST_RIGHT_INDEX,new QStandardItem(QString("Last")));
	
	ui.tableView->setModel(&modelTable);

	ui.tableView->setColumnWidth(0, 50);
	ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	ui.tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableView->setSelectionMode(QAbstractItemView::SingleSelection);


	QObject::connect(ui.pushButtonAddPair, SIGNAL(clicked()), this, SLOT(slotOnClickAddButton()));
	QObject::connect(ui.pushButtonRemoveSelected, SIGNAL(clicked()), this, SLOT(slotOnClickRemoveButton()));

    QObject::connect(ui.pushButtonBuyMkt, &QPushButton::clicked, this, &PairTradingGui::slotOnClickedButtonBuyMkt);
    QObject::connect(ui.pushButtonSellMkt, &QPushButton::clicked, this, &PairTradingGui::slotOnClickedButtonSellMkt);
  
    QObject::connect(ui.tableView, &QTableView::clicked, this, &PairTradingGui::slotOnClickedTableView);
    QObject::connect(ui.tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PairTradingGui::slotOnSelectionChanged);
    
    ui.verticalLayout_Tab->addWidget(&chartPair, 0, 0);
//    ui.verticalLayout_Tab->addWidget(&chartSpread, 0, 0);

    
    //temp
    CHistoricalData _data;
    _data.setClose(20.0);
    _data.setOpen(15.0);
    _data.setHigh(22.23);
    _data.setLow(14.5);

    candleChart.AddData(_data);
    ui.verticalLayout_Tab->addWidget(&candleChart, 0, 0);

}

PairTradingGui::~PairTradingGui()
{

}

// Add new Row in Table
void PairTradingGui::slotOnClickAddButton()
{
    QString textL = ui.lineEdit->text();
    QString textR = ui.lineEdit_2->text();

	bool isConditionOk = false;
	

	//remove all spaces from left and right
	textL = textL.trimmed();
	textR = textR.trimmed();


	if ((false == textL.isEmpty()) && (false == textR.isEmpty()))
	{
		textL = textL.toUpper();
		textR = textR.toUpper();

		if (textL != textR)
		{
			//check if pare presents in the table
			bool isFound = isParePresents(textL, textR);

			if (false == isFound)
			{
				isConditionOk = true;
			}
		}
	}

	if (isConditionOk)
	{
		int nr = this->modelTable.rowCount();
        rowUpdateMap_t retMap;
        QString llast("0");
        QString lbid("0");
        QString lask("0");
        QString rlast("0");
        QString rbid("0");
        QString rask("0");
        qint32 retKey = -1;

        //Check if symbol present in table
        if (true == isSymbolFoundInTable(textL, retMap))
        {
            retKey = retMap.firstKey();
            if ((0 <= retKey) && (nr > retKey))
            {
                //take last values from the same symbol
                llast = this->modelTable.item(retKey, LAST_LEFT_INDEX)->text();
                lbid = this->modelTable.item(retKey, BID_LEFT_INDEX)->text();
                lask = this->modelTable.item(retKey, ASK_LEFT_INDEX)->text();
            }
        }
        //Check if symbol present in table
        if (true == isSymbolFoundInTable(textR, retMap))
        {
            retKey = retMap.firstKey();
            if ((0 <= retKey) && (nr > retKey))
            {
                //take last values from the same symbol
                rlast = this->modelTable.item(retKey, LAST_RIGHT_INDEX)->text();
                rbid = this->modelTable.item(retKey, BID_RIGHT_INDEX)->text();
                rask = this->modelTable.item(retKey, ASK_RIGHT_INDEX)->text();
            }
        }

        //Add new Row to table
		this->modelTable.appendRow(new QStandardItem(QString("0")));
        this->modelTable.setItem(nr, LAST_LEFT_INDEX, new QStandardItem(llast));
        this->modelTable.setItem(nr, BID_LEFT_INDEX, new QStandardItem(lbid));
		this->modelTable.setItem(nr, ASK_LEFT_INDEX, new QStandardItem(lask));
		this->modelTable.setItem(nr, LEFT_INDEX, new QStandardItem(textL));
		this->modelTable.setItem(nr, RIGHT_INDEX, new QStandardItem(textR));
		this->modelTable.setItem(nr, BID_RIGHT_INDEX, new QStandardItem(rbid));
		this->modelTable.setItem(nr, ASK_RIGHT_INDEX, new QStandardItem(rask));
		this->modelTable.setItem(nr, LAST_RIGHT_INDEX, new QStandardItem(rlast));

        ui.tableView->selectRow(nr);
        ui.tableView->selectionModel()->select(ui.tableView->currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        m_activeTableIndex = nr;

        //send pair to 
        emit signalAddNewPair(textL, textR);
	}

    

    //////////////////////////////////////////////////////////////////////////
   // sendGetHistoryForPair();
    //TEMP
    //sendGetHistoryRequestToPM(textL);
   // Sleep(500);
    //sendGetHistoryRequestToPM(textR);


}


//************************************
// Method:    slotOnClickRemoveButton
// FullName:  PairTradingGui::slotOnClickRemoveButton
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void PairTradingGui::slotOnClickRemoveButton()
{
	QItemSelection selection(ui.tableView->selectionModel()->selection());

	QList<int> rows;
	foreach(const QModelIndex & index, selection.indexes()) {
		rows.append(index.row());
	}

	qSort(rows);

	int prev = -1;
	for (int i = rows.count() - 1; i >= 0; i -= 1) 
	{
		int current = rows[i];
		if (current != prev) 
		{
            const QString & refL = this->modelTable.item(current, LEFT_INDEX)->text();
            const QString & refR = this->modelTable.item(current, RIGHT_INDEX)->text();

            //Remove Row
            

            //beginRemoveRows(rows, current, current);
            this->modelTable.removeRows(current, 1);
            //endRemoveRows();
			prev = current;

            //Stop Data subscription 
            sendCancelRequestToPM(refL);
            sendCancelRequestToPM(refR);

		}
	}

    //m_activeTableIndex = getSelectedRowFromTableView();
   // sendGetHistoryForPair();
}




//************************************
// Method:    isParePresents
// FullName:  PairTradingGui::isParePresents
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const QString & s1
// Parameter: const QString & s2
//************************************
bool PairTradingGui::isParePresents(const QString & s1, const QString & s2)
{

	int nr = this->modelTable.rowCount();
	bool isFound = false;

	//check if 
	for (qint32 i = 0; ((i < nr) && (!isFound)); ++i)
	{
		const QString & refL = this->modelTable.item(i, LEFT_INDEX)->text();
		const QString & refR = this->modelTable.item(i, RIGHT_INDEX)->text();

		if (((s1 == refL) || (s1 == refR))
			&& ((s2 == refL) || (s2 == refR)))
		{
			isFound = true;;
		}
	}

	return isFound;
}

//************************************
// Method:    slotOnRealTimeTickData
// FullName:  PairTradingGui::slotOnRealTimeTickData
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const NS::CMyTickPrice & _pTickPrice
// Parameter: const QString & _symbol
//************************************
void PairTradingGui::slotOnRealTimeTickData(const IBDataTypes::CMyTickPrice & _pTickPrice, const QString & _symbol)
{
    
    rowUpdateMap_t retMap;
    qint32 nr = this->modelTable.rowCount();
    ColumIndexType ci = DEFAULT_INDEX_NONE;

    //m_pLog.AddLogMsg("--->>>>> [%s] symbol = %s, id = %d, price = %f, type = %d", __FUNCTION__, _symbol.toLocal8Bit().data(), _pTickPrice.getId(), _pTickPrice.getPrice(), _pTickPrice.getTickType());
    qCDebug(pairTraderGuiLog(), "--->>>>> symbol = %s, id = %d, price = %f, type = %d", _symbol.toLocal8Bit().data(), _pTickPrice.getId(), _pTickPrice.getPrice(), _pTickPrice.getTickType());
    if (true == isSymbolFoundInTable(_symbol, retMap))
    {
        rowUpdateMap_t::iterator it;
        for (it = retMap.begin(); it != retMap.end(); ++it)
        {
            if ((0 <= it.key()) && (nr > it.key()))
            {

                switch (_pTickPrice.getTickType())
                {
                case TickType::LAST:
                    if (it.value() == LEFT_INDEX)
                    {
                        ci = LAST_LEFT_INDEX;
                    }
                    else if (it.value() == RIGHT_INDEX)
                    {
                        ci = LAST_RIGHT_INDEX;
                    }

                    break;
                case TickType::ASK:

                    if (it.value() == LEFT_INDEX)
                    {
                        ci = ASK_LEFT_INDEX;
                    }
                    else if (it.value() == RIGHT_INDEX)
                    {
                        ci = ASK_RIGHT_INDEX;
                    }


                    break;
                case TickType::BID:
                    if (it.value() == LEFT_INDEX)
                    {
                        ci = BID_LEFT_INDEX;
                    }
                    else if (it.value() == RIGHT_INDEX)
                    {
                        ci = BID_RIGHT_INDEX;
                    }

                    break;

                default:

                    break;
                }

                if (DEFAULT_INDEX_NONE != ci)
                {
                    this->modelTable.setItem(it.key(), ci, new QStandardItem(QString::number(_pTickPrice.getPrice())));
                }
            }
        }
    }
    
}
//------------------------------------------------------------------------------
void PairTradingGui::slotOnFinishHistoricalData(const QList<CHistoricalData> & _pT, const QString _s1)
{
    //Add to graph
    //int i = ui.tableView->selectionModel()->currentIndex().row();

    //ToDo: check current row
    if ((m_activeTableIndex >= 0) && (this->modelTable.rowCount() > m_activeTableIndex))
    {
        const QString & refL = this->modelTable.item(m_activeTableIndex, LEFT_INDEX)->text();
        const QString & refR = this->modelTable.item(m_activeTableIndex, RIGHT_INDEX)->text();

        if (refL == _s1)
        {
            chartPair.AddSeries(_pT);
        }
        else if (refR == _s1)
        {
            //chartSpread.AddSeries(_pT);
            candleChart.AddSeries(_pT);
        }
    }

}

//------------------------------------------------------------------------------
void PairTradingGui::slotOnClickedButtonBuyMkt()
{
    QString textOrderSymbol = ui.lineEditOrderSymbol->text();
    //remove all spaces from left and right
    textOrderSymbol = textOrderSymbol.trimmed();
    if (!textOrderSymbol.isEmpty())
    {
        textOrderSymbol = textOrderSymbol.toUpper();
    }

    quint32 quantity = static_cast<quint32>(ui.spinBoxQuantity->value());

    emit signalRequestSendBuyMkt(textOrderSymbol, quantity);
}

//------------------------------------------------------------------------------
void PairTradingGui::slotOnClickedButtonSellMkt()
{
    QString textOrderSymbol = ui.lineEditOrderSymbol->text();
    //remove all spaces from left and right
    textOrderSymbol = textOrderSymbol.trimmed();
    if (!textOrderSymbol.isEmpty())
    {
        textOrderSymbol = textOrderSymbol.toUpper();
    }

    quint32 quantity = static_cast<quint32>(ui.spinBoxQuantity->value());

    emit signalRequestSendSellMkt("SPY", 100);
}

//------------------------------------------------------------------------------
void PairTradingGui::slotOnClickedTableView(const QModelIndex &index)
{
    if (index.isValid()) {
        
        m_activeTableIndex = index.row();
        //sendGetHistoryForPair();
    }
}
//------------------------------------------------------------------------------
void PairTradingGui::slotOnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    if (!selected.isEmpty())
    {
        if (m_activeTableIndex != selected.indexes().last().row())
        {
            m_activeTableIndex = selected.indexes().last().row();

            sendGetHistoryForPair();
        }
    }
    else
    {
        m_activeTableIndex = -1;
    }
}

//************************************
// Method:    isSymbolCanBeRemoved
// FullName:  PairTradingGui::isSymbolCanBeRemoved
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const QString & s1
//************************************
bool PairTradingGui::isSymbolCanBeRemoved(const QString & s1)
{
    int nr = this->modelTable.rowCount();
    qint32 foundCount = 0;
    bool ret = true;

    //check if 
    for (qint32 i = 0; i < nr; ++i)
    {
        const QString & refL = this->modelTable.item(i, LEFT_INDEX)->text();
        const QString & refR = this->modelTable.item(i, RIGHT_INDEX)->text();

        if ((s1 == refL) || (s1 == refR))
        {
            foundCount++;
        }
    }

    if (0 < foundCount)
    {
        ret = false;
    }

    return ret;
}

//************************************
// Method:    sendCancelRequestToPM
// FullName:  PairTradingGui::sendCancelRequestToPM
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: const QString _symbol
//************************************
void PairTradingGui::sendCancelRequestToPM(const QString _symbol)
{
    bool isRet = isSymbolCanBeRemoved(_symbol);
    //if (true == isRet)
    {
        emit signalRemoveSimbol(_symbol);
    }
}


//************************************
// Method:    isSymbolFoundInTable
// FullName:  PairTradingGui::isSymbolFoundInTable
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const QString & _s1
// Parameter: rowUpdateMap_t & _refMap
//************************************
bool PairTradingGui::isSymbolFoundInTable(const QString & _s1, rowUpdateMap_t & _refMap)
{
    qint32 nr = this->modelTable.rowCount();
    bool ret = false;

    //check if 
    for (qint32 i = 0; i < nr; ++i)
    {
        const QString & refL = this->modelTable.item(i, LEFT_INDEX)->text();
        const QString & refR = this->modelTable.item(i, RIGHT_INDEX)->text();

        if (_s1 == refL)
        {
            _refMap.insert(i, LEFT_INDEX);
        }
        else if (_s1 == refR)
        {
            _refMap.insert(i, RIGHT_INDEX);
        }
    }

    if (0 < _refMap.size())
    {
        ret = true;
    }

    return ret;
}

//************************************
// Method:    closeEvent
// FullName:  PairTradingGui::closeEvent
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: QCloseEvent * event
//************************************
void PairTradingGui::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "PairTrader", "Quit?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        //qDebug() << "Yes was clicked";
        qCDebug(pairTraderGuiLog(), "Yes was clicked");
        onCloseDialog();
        event->accept();
    }
    else {
        //qDebug() << "Yes was *not* clicked";
        qCDebug(pairTraderGuiLog(), "Yes was *not* clicked");
        event->ignore();
    }

}

//************************************
// Method:    onCloseDialog
// FullName:  PairTradingGui::onCloseDialog
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void PairTradingGui::onCloseDialog()
{
    this->modelTable.clear();

    emit signalGUIClosed();
}

//************************************
// Method:    getSelectedRowFromTableView
// FullName:  PairTradingGui::getSelectedRowFromTableView
// Access:    private 
// Returns:   QT_NAMESPACE::qint32
// Qualifier:
//************************************
qint32 PairTradingGui::getSelectedRowFromTableView()
{
    qint32 ret = -1;
    QItemSelection selection(ui.tableView->selectionModel()->selection());

    foreach(const QModelIndex & index, selection.indexes()) {
        ret = index.row();
    }

    return ret;
}

//************************************
// Method:    sendGetHistoryRequestToPM
// FullName:  PairTradingGui::sendGetHistoryRequestToPM
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: const QString _symbol
//************************************
void PairTradingGui::sendGetHistoryRequestToPM(const QString _symbol)
{
    emit signalRequestHistory(_symbol);
}

//************************************
// Method:    sendGetHistoryForPair
// FullName:  PairTradingGui::sendGetHistoryForPair
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void PairTradingGui::sendGetHistoryForPair()
{
    if ((m_activeTableIndex >= 0) && (this->modelTable.rowCount() > m_activeTableIndex))
    {
        const QString & refL = this->modelTable.item(m_activeTableIndex, LEFT_INDEX)->text();
        const QString & refR = this->modelTable.item(m_activeTableIndex, RIGHT_INDEX)->text();
        sendGetHistoryRequestToPM(refL);
        sendGetHistoryRequestToPM(refR);
    }
}

