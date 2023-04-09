#include "CPortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include "caccount.h"
#include <QVariantMap>

#include "cteststrategy.h"
#include "cbasicportfolio.h"
#include "cbasicaccount.h"

#include "cstrategyfactory.h"

#define START_OF_WORKING_NODES (3u)

CPortfolioConfigModel::CPortfolioConfigModel(QTreeView *treeView, CBasicRoot *pRoot, QObject *parent)
    : CTreeViewCustomModel(treeView, parent)
    , m_pRoot(pRoot)
    , m_brokerInterface(nullptr)
    , m_UpdateInfoTimer(this)
{
    Q_UNUSED(parent);

    QObject::connect(&m_UpdateInfoTimer, &QTimer::timeout, this, &CPortfolioConfigModel::slotOnTimeoutCallback);
    m_UpdateInfoTimer.start(1000);
    setupModelData(rootItem);

}

TreeItem * CPortfolioConfigModel::addRootNode(TreeItem * parent, pItemDataType name, pItemDataType value, int columnCount)
{
    TreeItem * _parent = parent;

    _parent->insertChildren(_parent->childCount(), 1, columnCount);
    _parent->child(_parent->childCount() - 1)->addData(0, name);
    _parent->child(_parent->childCount() - 1)->addData(1, value);
    return parent;
}

//void addParameters(TreeItem * parent, const QVariantMap params, int columnCount)
//{
//    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
//    if(!params.empty())
//    {
//        //Create account paramters
//        TreeItem * parentParameters = addRootNode(parent->child(parent->childCount() - 1),
//                                       pItemDataType(new stItemData("Parameters", EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
//                                       EmptyRoItem,
//                                       columnCount);

//        TreeItem * _parent = parentParameters->child(parentParameters->childCount() - 1);
//        for (auto i = params.begin(); i != params.end(); ++i)
//        {
//            _parent->insertChildren(_parent->childCount(), 1, columnCount);
//            _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVT_RO_TEXT, PM_ITEM_PARAMETER)));
//            switch (i.value().typeId()) {
//            case QMetaType::Double:
//            case QMetaType::Float:
//            case QMetaType::Float16:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toDouble(), EVT_TEXT, TVM_UNUSED_ID)));
//                break;
//            case QMetaType::Int:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toInt(), EVT_TEXT, TVM_UNUSED_ID)));
//                break;
//            case QMetaType::QString:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_TEXT, TVM_UNUSED_ID)));
//                break;
//            default:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_TEXT, TVM_UNUSED_ID)));
//                break;
//            }

//        }
//    }
//}

//void addGenericInfo(TreeItem * parent, const QVariantMap params, int columnCount)
//{
//    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
//    if(!params.empty())
//    {
//        //Create account paramters
//        TreeItem * parentInfo = addRootNode(parent->child(parent->childCount() - 1),
//                                                 pItemDataType(new stItemData("Info", EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
//                                                 EmptyRoItem,
//                                                 columnCount);

//        TreeItem * _parent = parentInfo->child(parentInfo->childCount() - 1);
//        for (auto i = params.begin(); i != params.end(); ++i)
//        {
//            _parent->insertChildren(_parent->childCount(), 1, columnCount);
//            _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVT_RO_TEXT, PM_ITEM_PARAMETER)));
//            switch (i.value().typeId()) {
//            case QMetaType::Double:
//            case QMetaType::Float:
//            case QMetaType::Float16:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QString::number(i.value().toDouble(), 'f', 2), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                break;
//            case QMetaType::Int:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toInt(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                break;
//            case QMetaType::QString:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                break;
//            default:
//                _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                break;
//            }

//        }
//    }
//}

//void addAssetInfo(TreeItem * parent, const QVariantMap params, int columnCount)
//{
//    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
//    if(!params.empty())
//    {
//        //Create account paramters
//        TreeItem * parentInfo = addRootNode(parent->child(parent->childCount() - 1),
//                                           pItemDataType(new stItemData("Assets", EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
//                                           EmptyRoItem,
//                                           columnCount);

//        //TreeItem * _parent = parentInfo->child(parentInfo->childCount() - 1);

//        for (auto z = params.begin(); z != params.end(); ++z)
//        {
//            TreeItem * _parentNode = addRootNode(parentInfo->child(parentInfo->childCount() - 1),
//                                            pItemDataType(new stItemData(z.key(), EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
//                                            EmptyRoItem,
//                                            columnCount);

//            QVariantMap tmpMap = z.value().toMap();

//            if(!tmpMap.empty())
//            {
//                //Create account paramters
//                TreeItem * _parent = _parentNode->child(_parentNode->childCount() - 1);
//                for (auto i = tmpMap.begin(); i != tmpMap.end(); ++i)
//                {
//                    _parent->insertChildren(_parent->childCount(), 1, columnCount);
//                    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVT_RO_TEXT, PM_ITEM_PARAMETER)));
//                    switch (i.value().typeId()) {
//                    case QMetaType::Double:
//                    case QMetaType::Float:
//                    case QMetaType::Float16:
//                        _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QString::number(i.value().toDouble(), 'f', 2), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                        break;
//                    case QMetaType::Int:
//                        _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toInt(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                        break;
//                    case QMetaType::QString:
//                        _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                        break;
//                    default:
//                        _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toString(), EVT_RO_TEXT, TVM_UNUSED_ID)));
//                        break;
//                    }

//                }
//            }
//        }
//    }
//}


void CPortfolioConfigModel::addDataToNode(TreeItem *parent, const QString &key, const QVariant &value, int typeId, bool readOnly, int columnCount)
{
    parent->insertChildren(parent->childCount(), 1, columnCount);
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(key, EVT_RO_TEXT, PM_ITEM_PARAMETER)));

    int eventType = readOnly ? EVT_RO_TEXT : EVT_TEXT;
    switch (typeId)
    {
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::Float16:
        parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QString::number(value.toDouble(), 'f', 2), eventType, TVM_UNUSED_ID)));
        break;
    case QMetaType::Int:
        parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(value.toInt(), eventType, TVM_UNUSED_ID)));
        break;
    default:
        parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(value.toString(), eventType, TVM_UNUSED_ID)));
        break;
    }
}

//void addNestedNodes(TreeItem *parent, const QString &rootName, const QVariantMap &params, bool readOnly, int columnCount)
//{
//    if (!params.empty())
//    {
//        auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
//        TreeItem *parentNode = addRootNode(parent->child(parent->childCount() - 1),
//                                           pItemDataType(new stItemData(rootName, EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
//                                           EmptyRoItem,
//                                           2);

//        TreeItem *_parent = parentNode->child(parentNode->childCount() - 1);
//        for (auto i = params.begin(); i != params.end(); ++i)
//        {
//            if (i.value().typeId() == QMetaType::QVariantMap)
//            {
//                QVariantMap nestedMap = i.value().toMap();
//                addNestedNodes(_parent, i.key(), nestedMap, readOnly, columnCount);
//            }
//            else
//            {
//                addDataToNode(_parent, i.key(), i.value(), i.value().typeId(), readOnly, columnCount);
//            }
//        }
//    }
//}

void CPortfolioConfigModel::addNestedNodes(TreeItem *parent, const QString &rootName, const QVariantMap &params, bool readOnly, int columnCount)
{
    if (!params.empty())
    {
        auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
        TreeItem *parentNode = addRootNode(parent->child(parent->childCount() - 1),
                                           pItemDataType(new stItemData(rootName, EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
                                           EmptyRoItem,
                                           columnCount);

        TreeItem *_parent = parentNode->child(parentNode->childCount() - 1);
        for (auto i = params.begin(); i != params.end(); ++i)
        {
            if (i.value().typeId() == QMetaType::QVariantMap)
            {
                QVariantMap nestedMap = i.value().toMap();
                if(!nestedMap.empty())
                {
                    addNestedNodes(parentNode, i.key(), nestedMap, readOnly, columnCount);
                }
            }
            else
            {
                addDataToNode(_parent, i.key(), i.value(), i.value().typeId(), readOnly, columnCount);
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

        //addParameters(parentAccount, account->getParameters(), rootItem->columnCount());
        addNestedNodes(parentAccount, "Parameters", account->getParameters(), false, rootItem->columnCount());

        for (auto&& portfolio : account->getModels()) {
            TreeItem * parentPortfolio = addRootNode(parentAccount->child(parentAccount->childCount() - 1),
                                           pItemDataType(new stItemData(portfolio->getName(), EVT_RO_TEXT, PM_ITEM_PORTFOLIO)),
                                           EmptyRoItem,
                                           rootItem->columnCount());

            //addParameters(parentPortfolio, portfolio->getParameters(), rootItem->columnCount());
            addNestedNodes(parentPortfolio, "Parameters", portfolio->getParameters(), false, rootItem->columnCount());

            for (auto&& strategy : portfolio->getModels()) {
                TreeItem * parentStrategy = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
                                               pItemDataType(new stItemData(strategy->getName(), EVT_RO_TEXT, PM_ITEM_STRATEGY)),
                                               EmptyRoItem,
                                               rootItem->columnCount());

                //addParameters(parentStrategy, strategy->getParameters(), rootItem->columnCount());
                addNestedNodes(parentStrategy, "Parameters", strategy->getParameters(), false, rootItem->columnCount());
            }

        }

    }

}

void CPortfolioConfigModel::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {

        auto valueIndex = topLeft;
        auto *itemToUpdate = getItem(topLeft);

        QList<quint16> Ids{PM_ITEM_STRATEGY, PM_ITEM_STRATEGIES, PM_ITEM_PORTFOLIO, PM_ITEM_ACCOUNT};
        auto index = findWorkingNode(topLeft, Ids);
        auto *tmpItem = getItem(index);

        auto updateModel = [&](auto modelToUpdate) {
            const auto itemId = itemToUpdate->data(0).id;
            const auto tmpItemId = tmpItem->data(0).id;

            if (itemId != tmpItemId)
            {
                auto paramMap = modelToUpdate->getParameters();
                const auto& key = itemToUpdate->data(0).value.toString();

                if (paramMap.contains(key))
                {
                    paramMap[key] = itemToUpdate->data(valueIndex.column()).value;
                    modelToUpdate->setParameters(paramMap);
                }
            }
            else
            {
                const auto columnIndex = valueIndex.column();

                if (columnIndex == 0)
                {
                    modelToUpdate->setName(itemToUpdate->data(0).value.toString());
                }
                else if (columnIndex == 1)
                {
                    const auto isActive = itemToUpdate->data(valueIndex.column()).value.toInt() == Qt::Checked;
                    isActive ? modelToUpdate->start() : modelToUpdate->stop();
                }
            }
        };

        switch (tmpItem->data(0).id) {
        case PM_ITEM_ACCOUNT:
        {
            auto accountToUpdate = m_pRoot->getModels().value(index.row(), nullptr);
            if (accountToUpdate) {
                updateModel(accountToUpdate);
            }
        }
        break;
        case PM_ITEM_PORTFOLIO:
        {
            auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
            auto portfolioToUpdate = account ? account->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            if (portfolioToUpdate) {
                updateModel(portfolioToUpdate);
            }
        }
        break;
        case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio = account ? account->getModels().value(index.parent().row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            auto strategyToUpdate = portfolio ? portfolio->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            if (strategyToUpdate) {
                updateModel(strategyToUpdate);
            }
        }
        break;
        default:
            break;
        }
    }
}

void CPortfolioConfigModel::addModel(const QModelIndex& index, const QList<quint16>& ids, quint32 itemType)
{
    QModelIndex workingIndex = findWorkingNode(index, ids);
    TreeItem* item = getItem(workingIndex);
    ptrGenericModelType model = nullptr;

    if (item->data(0).id == ids.first()) {
        switch (itemType) {
        case PM_ITEM_ACCOUNT:
            {
                model = QSharedPointer<CBasicAccount>::create();
                model->setName("Account");
                m_pRoot->addModel(model);
            }
            break;
        case PM_ITEM_PORTFOLIO:
            {
                auto account = m_pRoot->getModels().value(workingIndex.row(), nullptr);
                if(nullptr != account)
                {
                    model = QSharedPointer<CBasicPortfolio>::create();
                    model->setName("Portfolio");
                    account->addModel(model);
                }
            }
            break;
        case PM_ITEM_STRATEGY:
            {
                auto account = m_pRoot->getModels().value(workingIndex.parent().row(), nullptr);
                auto portfolio = account ? account->getModels().value(workingIndex.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
                if((nullptr != portfolio) && (nullptr != this->m_brokerInterface))
                {
                    model = CStrategyFactory::createNewStrategy(STRATEGY_MA);
                    //model = CStrategyFactory::createNewStrategy(STRATEGY_BASIC_TEST);
                    model->setBrokerInterface(this->m_brokerInterface);
                    portfolio->addModel(model);
                }
            }
            break;
        default:
            break;
        }

        if(nullptr != model)
        {
            addWorkingNode(workingIndex, model, itemType);
        }
    }
}

void CPortfolioConfigModel::slotOnClickAddAccount()
{
    addModel(createIndex(0, 0, rootItem->child(rootItem->childCount() - 1)), {PM_ITEM_ACCOUNTS}, PM_ITEM_ACCOUNT);
}

void CPortfolioConfigModel::slotOnClickAddPortfolio()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_ACCOUNT}, PM_ITEM_PORTFOLIO);
    }
}

void CPortfolioConfigModel::slotOnClickAddStrategy()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_PORTFOLIO}, PM_ITEM_STRATEGY);
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

    //addParameters(parentAccount, pModel->getParameters(), item->columnCount());
    addNestedNodes(parentAccount, "Parameters", pModel->getParameters(), false, item->columnCount());
    //addGenericInfo(parentAccount, pModel->genericInfo(), item->columnCount());
    addNestedNodes(parentAccount, "Info", pModel->genericInfo(), true, item->columnCount());
    //addAssetInfo(parentAccount, pModel->assetList(), item->columnCount());
    addNestedNodes(parentAccount, "Assets", pModel->assetList(), true, item->columnCount());

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

void CPortfolioConfigModel::slotOnTimeoutCallback()
{
    traverseNodes(rootItem);
}


void CPortfolioConfigModel::removeModel(QModelIndex index)
{
    TreeItem *tmpItem = getItem(index);

    switch (tmpItem->data(0).id) {
    case PM_ITEM_ACCOUNT:
        {
            auto accountToRemove = m_pRoot->getModels().value(index.row(), nullptr);
            if(nullptr != accountToRemove)
                m_pRoot->removeModel(accountToRemove);
        }
        break;
    case PM_ITEM_PORTFOLIO:
        {
            auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
            auto portfolioToRemove = account ? account->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            if(nullptr != portfolioToRemove)
                account->removeModel(portfolioToRemove);
        }
        break;
    case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio = account ? account->getModels().value(index.parent().row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            auto strategyToRemove = portfolio ? portfolio->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            if(nullptr != strategyToRemove)
                portfolio->removeModel(strategyToRemove);
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


void CPortfolioConfigModel::traverseNodes(TreeItem *node)
{
    processNode(node); // Process the current node

    // Recursively visit all childrenf
    for (int i = 0; i < node->childCount(); ++i) {
        TreeItem* child = node->child(i);
        traverseNodes(child);
    }
}

void CPortfolioConfigModel::processNode(TreeItem *node)
{
    stItemData data = node->data(0);
    qDebug() << "Node data (column 0):" << data.value;

    emit signalUpdateDataAll();
}


