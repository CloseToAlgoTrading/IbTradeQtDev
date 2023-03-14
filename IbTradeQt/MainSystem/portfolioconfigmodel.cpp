#include "PortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include "caccount.h"
#include <QVariantMap>

#include "cteststrategy.h"
#include "cbasicportfolio.h"
#include "cbasicaccount.h"

PortfolioConfigModel::PortfolioConfigModel(QTreeView *treeView, QObject *parent)
    : CTreeViewCustomModel(treeView, parent)
    , m_Root()
{
    Q_UNUSED(parent);

//    CTestStrategy *pS1 = new CTestStrategy();
//    pS1->setName("S1");
//    CTestStrategy *pS2 = new CTestStrategy();
//    pS2->setName("S2");
//    CTestStrategy *pS3 = new CTestStrategy();
//    pS3->setName("S3");
//    CBasicPortfolio *pP1 = new CBasicPortfolio();
//    pP1->setName("P1");
//    CBasicPortfolio *pP2 = new CBasicPortfolio();
//    pP2->setName("P2");
//    CBasicAccount *pA1 = new CBasicAccount();
//    pA1->setName("A1");
//    CBasicAccount *pA2 = new CBasicAccount();
//    pA2->setName("A2");

//    pP1->addModel(static_cast<ptrGenericModelType>(pS1));
//    pP1->addModel(static_cast<ptrGenericModelType>(pS2));

//    pP2->addModel(static_cast<ptrGenericModelType>(pS3));

//    pA1->addModel(static_cast<ptrGenericModelType>(pP1));
//    pA1->addModel(static_cast<ptrGenericModelType>(pP2));

//    m_Root.addModel(static_cast<ptrGenericModelType>(pA1));
//    m_Root.addModel(static_cast<ptrGenericModelType>(pA2));

    setupModelData(rootItem);
}

TreeItem * addRootNode(TreeItem * parent, pItemDataType name, pItemDataType value, int columnCount)
{
    TreeItem * _parent = parent;

    _parent->insertChildren(_parent->childCount(), 1, columnCount);
    _parent->child(_parent->childCount() - 1)->addData(0, name);
    _parent->child(_parent->childCount() - 1)->addData(1, value);
    return parent;
}

void addParameters(TreeItem * parent, const QVariantMap params, int columnCount)
{
    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVET_RO_TEXT, TVM_UNUSED_ID));
    if(!params.empty())
    {
        //Create account paramters
        TreeItem * parentParameters = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, PM_ITEM_PARAMETERS)),
                                       EmptyRoItem,
                                       columnCount);

        TreeItem * _parent = parentParameters->child(parentParameters->childCount() - 1);
        for (auto i = params.begin(); i != params.end(); ++i)
        {
            _parent->insertChildren(_parent->childCount(), 1, columnCount);
            _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVET_RO_TEXT, PM_ITEM_PARAMETER)));
            switch (i.value().typeId()) {
            case QMetaType::Double:
                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toDouble(), EVT_TEXT, TVM_UNUSED_ID)));
                break;
            case QMetaType::Int:
                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toInt(), EVT_TEXT, TVM_UNUSED_ID)));
                break;
            case QMetaType::QString:
                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_TEXT, TVM_UNUSED_ID)));
                break;
            default:
                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_TEXT, TVM_UNUSED_ID)));
                break;
            }

        }
    }
}

void PortfolioConfigModel::setupModelData(TreeItem * rootItem)
{
    CAccount myMap;

    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVET_RO_TEXT, TVM_UNUSED_ID));

    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, TVM_UNUSED_ID)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, TVM_UNUSED_ID)));

    QList<TreeItem * > parents;
    parents << rootItem;

    //Create an Accounts
    TreeItem * parent = addRootNode(parents.last(),
                                   pItemDataType(new stItemData("Accounts", EVET_RO_TEXT, PM_ITEM_ACCOUNTS)),
                                   EmptyRoItem,
                                   rootItem->columnCount());

    //*+++++++++++++++++++++++++
    for (auto&& account : this->m_Root.getModels()) {
        TreeItem * parentAccount = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData(account->getName(), EVET_RO_TEXT, PM_ITEM_ACCOUNT)),
                                       EmptyRoItem,
                                       rootItem->columnCount());

        addParameters(parentAccount, account->getParameters(), rootItem->columnCount());

        for (auto&& portfolio : account->getModels()) {
            TreeItem * parentPortfolio = addRootNode(parentAccount->child(parentAccount->childCount() - 1),
                                           pItemDataType(new stItemData(portfolio->getName(), EVET_RO_TEXT, PM_ITEM_PORTFOLIO)),
                                           EmptyRoItem,
                                           rootItem->columnCount());

            addParameters(parentPortfolio, portfolio->getParameters(), rootItem->columnCount());

            for (auto&& strategy : portfolio->getModels()) {
                TreeItem * parentStrategy = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
                                               pItemDataType(new stItemData(strategy->getName(), EVET_RO_TEXT, PM_ITEM_STRATEGY)),
                                               EmptyRoItem,
                                               rootItem->columnCount());

                addParameters(parentStrategy, strategy->getParameters(), rootItem->columnCount());
            }

        }

    }

}

void PortfolioConfigModel::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(topLeft.internalPointer());
        if(item)
        {
            const auto itemData = item->data(topLeft.column());
        }
    }
}


void PortfolioConfigModel::onClickAddNodeButton()
{
    static quint8 i = 0;
    //CTestStrategy *pS1 = new CTestStrategy();
    //pS1->setName("S1");
    //    CTestStrategy *pS2 = new CTestStrategy();
    //    pS2->setName("S2");
    //    CTestStrategy *pS3 = new CTestStrategy();
    //    pS3->setName("S3");
    //    CBasicPortfolio *pP1 = new CBasicPortfolio();
    //    pP1->setName("P1");
    //    CBasicPortfolio *pP2 = new CBasicPortfolio();
    //    pP2->setName("P2");
    CBasicAccount *pA1 = new CBasicAccount();
    pA1->setName("A"+QString::number(i++));
    //    CBasicAccount *pA2 = new CBasicAccount();
    //    pA2->setName("A2");

    //    pP1->addModel(static_cast<ptrGenericModelType>(pS1));
    //    pP1->addModel(static_cast<ptrGenericModelType>(pS2));

    //    pP2->addModel(static_cast<ptrGenericModelType>(pS3));

    //    pA1->addModel(static_cast<ptrGenericModelType>(pP1));
    //    pA1->addModel(static_cast<ptrGenericModelType>(pP2));

    m_Root.addModel(static_cast<ptrGenericModelType>(pA1));
    //    m_Root.addModel(static_cast<ptrGenericModelType>(pA2));

    beginInsertRows(QModelIndex(),rootItem->childCount(),rootItem->childCount());

    TreeItem * parentAccount = addRootNode(rootItem->child(rootItem->childCount() - 1),
                                   pItemDataType(new stItemData(pA1->getName(), EVET_RO_TEXT, PM_ITEM_ACCOUNT)),
                                   pItemDataType(new stItemData(QVariant(), EVET_RO_TEXT, TVM_UNUSED_ID)),
                                   rootItem->columnCount());

    addParameters(parentAccount, pA1->getParameters(), rootItem->columnCount());

    endInsertRows();


    QModelIndex index = createIndex(0,0,parentAccount);
    emit signalUpdateData(index);

}

void PortfolioConfigModel::onClickRemoveNodeButton()
{
    QModelIndexList indexes = m_treeView->selectionModel()->selectedIndexes();
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();



    if (selectionModel->hasSelection()) {
        QModelIndexList indexes = selectionModel->selectedIndexes();
        QModelIndex index = indexes.at(0); // Assumes single selection mode

        TreeItem * tmpItem = getItem(index);
        beginRemoveRows(index.parent(), index.row(), index.row() );

        //tmpItem->removeChildren(0, tmpItem->childCount());
        TreeItem * tmpParemt = tmpItem->parent();
        tmpParemt->removeChildren(tmpItem->childNumber(), 1);


        endRemoveRows();
        emit signalUpdateData(createIndex(0,0,rootItem));
    }
}
