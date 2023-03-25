#include "CPortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include "caccount.h"
#include <QVariantMap>

#include "cteststrategy.h"
#include "cbasicportfolio.h"
#include "cbasicaccount.h"

#include "cstrategyfactory.h"

CPortfolioConfigModel::CPortfolioConfigModel(QTreeView *treeView, CBasicRoot *pRoot, QObject *parent)
    : CTreeViewCustomModel(treeView, parent)
    , m_pRoot(pRoot)
{
    Q_UNUSED(parent);

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
    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
    if(!params.empty())
    {
        //Create account paramters
        TreeItem * parentParameters = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData("Parameters", EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
                                       EmptyRoItem,
                                       columnCount);

        TreeItem * _parent = parentParameters->child(parentParameters->childCount() - 1);
        for (auto i = params.begin(); i != params.end(); ++i)
        {
            _parent->insertChildren(_parent->childCount(), 1, columnCount);
            _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVT_RO_TEXT, PM_ITEM_PARAMETER)));
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

void CPortfolioConfigModel::setupModelData(TreeItem * rootItem)
{
    CAccount myMap;

    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));

    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, TVM_UNUSED_ID)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, TVM_UNUSED_ID)));

    QList<TreeItem * > parents;
    parents << rootItem;

    //Create an Accounts
    TreeItem * parent = addRootNode(parents.last(),
                                   pItemDataType(new stItemData("Accounts", EVT_RO_TEXT, PM_ITEM_ACCOUNTS)),
                                   EmptyRoItem,
                                   rootItem->columnCount());

    //*+++++++++++++++++++++++++
    for (auto&& account : this->m_pRoot->getModels()) {
        TreeItem * parentAccount = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData(account->getName(), EVT_RO_TEXT, PM_ITEM_ACCOUNT)),
                                       EmptyRoItem,
                                       rootItem->columnCount());

        addParameters(parentAccount, account->getParameters(), rootItem->columnCount());

        for (auto&& portfolio : account->getModels()) {
            TreeItem * parentPortfolio = addRootNode(parentAccount->child(parentAccount->childCount() - 1),
                                           pItemDataType(new stItemData(portfolio->getName(), EVT_RO_TEXT, PM_ITEM_PORTFOLIO)),
                                           EmptyRoItem,
                                           rootItem->columnCount());

            addParameters(parentPortfolio, portfolio->getParameters(), rootItem->columnCount());

            for (auto&& strategy : portfolio->getModels()) {
                TreeItem * parentStrategy = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
                                               pItemDataType(new stItemData(strategy->getName(), EVT_RO_TEXT, PM_ITEM_STRATEGY)),
                                               EmptyRoItem,
                                               rootItem->columnCount());

                addParameters(parentStrategy, strategy->getParameters(), rootItem->columnCount());
            }

        }

    }

}

void CPortfolioConfigModel::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {
        
        //XXXXXXXXXXXXXXXXXXX
        auto valueIndex = topLeft;
        TreeItem * itemToUpdate = getItem(topLeft);

        QList<quint16> Ids{PM_ITEM_STRATEGY, PM_ITEM_STRATEGIES, PM_ITEM_PORTFOLIO, PM_ITEM_ACCOUNT};
        auto index = findWorkingNode(topLeft, Ids);
        TreeItem * tmpItem = getItem(index);

        switch (tmpItem->data(0).id) {
        case PM_ITEM_ACCOUNT:
            {
                auto accountToUpdate = m_pRoot->getModels().value(index.row(), nullptr);
                if (accountToUpdate) {
                    if(itemToUpdate->data(0).id != PM_ITEM_ACCOUNT)
                    {
                        //update parameters
                        auto paramMap = accountToUpdate->getParameters();
                        const auto& key = itemToUpdate->data(0).value.toString();
                        if (paramMap.contains(key))
                        {
                            paramMap[key] = itemToUpdate->data(valueIndex.column()).value;
                            accountToUpdate->setParameters(paramMap);
                        }
                    }
                    else
                    {
                        if(valueIndex.column() == 0)
                            accountToUpdate->setName(itemToUpdate->data(0).value.toString());
                    }
                }
            }
            break;
        case PM_ITEM_PORTFOLIO:
            {
                auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
                auto portfolioToUpdate = account ? account->getModels().value(index.row() - 1, nullptr) : nullptr;
                if (portfolioToUpdate) {
                    if(itemToUpdate->data(0).id != PM_ITEM_PORTFOLIO)
                    {
                        //update parameters
                        auto paramMap = portfolioToUpdate->getParameters();
                        const auto& key = itemToUpdate->data(0).value.toString();
                        if (paramMap.contains(key))
                        {
                            paramMap[key] = itemToUpdate->data(valueIndex.column()).value;
                            portfolioToUpdate->setParameters(paramMap);
                        }
                    }
                    else
                    {
                        if(valueIndex.column() == 0)
                            portfolioToUpdate->setName(itemToUpdate->data(0).value.toString());
                        else if(valueIndex.column() == 1)
                        {
                            const auto isActive = itemToUpdate->data(valueIndex.column()).value.toInt() == Qt::Checked ? true : false;
                            if (isActive == true)
                            {
                                (void)portfolioToUpdate->start();
                            }
                            else
                            {
                                (void)portfolioToUpdate->stop();
                            }

                        }
                    }
                }
            }
            break;
        case PM_ITEM_STRATEGY:
            {
                auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
                auto portfolio = account ? account->getModels().value(index.parent().row() - 1, nullptr) : nullptr;
                auto strategyToUpdate = portfolio ? portfolio->getModels().value(index.row() - 1, nullptr) : nullptr;
                if (strategyToUpdate) {
                    if(itemToUpdate->data(0).id != PM_ITEM_STRATEGY)
                    {
                        //update parameters
                        auto paramMap = strategyToUpdate->getParameters();
                        const auto& key = itemToUpdate->data(0).value.toString();
                        if (paramMap.contains(key))
                        {
                            paramMap[key] = itemToUpdate->data(valueIndex.column()).value;
                            strategyToUpdate->setParameters(paramMap);
                        }
                    }
                    else
                    {
                        if(valueIndex.column() == 0)
                            strategyToUpdate->setName(itemToUpdate->data(0).value.toString());
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}

void CPortfolioConfigModel::addModel(const QModelIndex& index, const QList<quint16>& ids, ptrGenericModelType model, quint16 itemType)
{
    QModelIndex workingIndex = findWorkingNode(index, ids);
    TreeItem* item = getItem(workingIndex);

    if (item->data(0).id == ids.first()) {
        switch (itemType) {
        case PM_ITEM_ACCOUNT:
            {
                m_pRoot->addModel(model);
            }
            break;
        case PM_ITEM_PORTFOLIO:
            {
                auto account = m_pRoot->getModels().value(workingIndex.row(), nullptr);
                account->addModel(model);
            }
            break;
        case PM_ITEM_STRATEGY:
            {
                auto account = m_pRoot->getModels().value(workingIndex.parent().row(), nullptr);
                auto portfolio = account ? account->getModels().value(workingIndex.row() - 1, nullptr) : nullptr;
                portfolio->addModel(model);
            }
            break;
        default:
            break;
        }

        addWorkingNode(workingIndex, model, itemType);
    }
}

void CPortfolioConfigModel::slotOnClickAddAccount()
{
    static quint8 i = 0;
    auto pA1 = QSharedPointer<CBasicAccount>::create();
    pA1->setName("A" + QString::number(i++));
    QModelIndex rootIndex = createIndex(0, 0, rootItem->child(rootItem->childCount() - 1));
    addModel(rootIndex, {PM_ITEM_ACCOUNTS}, pA1, PM_ITEM_ACCOUNT);
}

void CPortfolioConfigModel::slotOnClickAddPortfolio()
{
    static quint16 i = 0;
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        QModelIndex index = selectionModel->currentIndex();
        auto pP1 = QSharedPointer<CBasicPortfolio>::create();
        pP1->setName("P" + QString::number(i++));
        addModel(index, {PM_ITEM_ACCOUNT}, pP1, PM_ITEM_PORTFOLIO);
    }
}

void CPortfolioConfigModel::slotOnClickAddStrategy()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        QModelIndex index = selectionModel->currentIndex();
        auto pS1 = CStrategyFactory::createNewStrategy(STRATEGY_BASIC_TEST);
        addModel(index, {PM_ITEM_PORTFOLIO}, pS1, PM_ITEM_STRATEGY);
    }
}
QModelIndex CPortfolioConfigModel::findWorkingNode(QModelIndex index, const QList<quint16> & Ids)
{
    TreeItem * tmpItem = getItem(index);
    const quint16 id = tmpItem->data(0).id;
    if(Ids.contains(id))
    {
        return index;
    }
    else
    {
      if(PM_ITEM_ACCOUNTS != id)
        return findWorkingNode(index.parent(), Ids);
    }
    return QModelIndex();
}

void CPortfolioConfigModel::addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id)
{
    TreeItem * item = getItem(index);
    beginInsertRows(index,item->childCount(),item->childCount());

    TreeItem * parentAccount = addRootNode(item,
                                   pItemDataType(new stItemData(pModel->getName(), EVT_TEXT, id)),
                                   pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, id + PT_ITEM_ACTIVATION)),
                                   item->columnCount());

    addParameters(parentAccount, pModel->getParameters(), item->columnCount());

    endInsertRows();

    QModelIndex newIndex = createIndex(index.row(), 0, parentAccount);
    emit signalUpdateData(newIndex);
}


void CPortfolioConfigModel::onClickRemoveNodeButton()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        QModelIndex index = selectionModel->currentIndex(); // Assumes single selection mode
        if(PM_ITEM_ACCOUNTS != getItem(index)->data(0).id)
        {
            QList<quint16> Ids{PM_ITEM_STRATEGY, PM_ITEM_STRATEGIES, PM_ITEM_PORTFOLIO, PM_ITEM_ACCOUNT};
            index = findWorkingNode(index, Ids);

            removeModel(index);
        }
    }
}


void CPortfolioConfigModel::removeModel(QModelIndex index)
{
    TreeItem *tmpItem = getItem(index);

//    auto removeModelFromList = [&](ptrGenericModelType modelToRemove, QList<ptrGenericModelType>& modelsList) {
//        if (modelToRemove) {
//            modelsList.removeOne(modelToRemove);
//        }
//    };

    switch (tmpItem->data(0).id) {
    case PM_ITEM_ACCOUNT:
        {
            auto accountToRemove = m_pRoot->getModels().value(index.row(), nullptr);
            m_pRoot->removeModel(accountToRemove);
            //removeModelFromList(accountToRemove, m_pRoot->getModels());
        }
        break;
    case PM_ITEM_PORTFOLIO:
        {
            auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
            auto portfolioToRemove = account ? account->getModels().value(index.row() - 1, nullptr) : nullptr;
            account->removeModel(portfolioToRemove);
            //removeModelFromList(portfolioToRemove, account->getModels());
        }
        break;
    case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio = account ? account->getModels().value(index.parent().row() - 1, nullptr) : nullptr;
            auto strategyToRemove = portfolio ? portfolio->getModels().value(index.row() - 1, nullptr) : nullptr;
            portfolio->removeModel(strategyToRemove);
            //removeModelFromList(strategyToRemove, portfolio->getModels());
        }
        break;
    default:
        break;
    }

    beginRemoveRows(index.parent(), index.row(), index.row());
    tmpItem->parent()->removeChildren(tmpItem->childNumber(), 1);
    endRemoveRows();
    emit signalUpdateData(createIndex(0, 0, rootItem));
}
