#include "CPortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include <QVariantMap>

#include "cbasicportfolio.h"
#include "cbasicaccount.h"

#include "cstrategyfactory.h"
#include "cbasicroot.h"

#include "ModelType.h"

#define START_OF_WORKING_NODES (3u)

CPortfolioConfigModel::CPortfolioConfigModel(QTreeView *treeView, CBasicRoot *pRoot, QObject *parent)
    : CTreeViewCustomModel(treeView, parent)
    , m_pRoot(pRoot)
    , m_brokerInterface(nullptr)
    , m_UpdateInfoTimer(this)
{
    Q_UNUSED(parent);

    QObject::connect(&m_UpdateInfoTimer, &QTimer::timeout, this, &CPortfolioConfigModel::slotOnTimeoutCallback);
    m_UpdateInfoTimer.start(100);
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
    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, TVM_UNUSED_ID)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, TVM_UNUSED_ID)));

    QList<TreeItem * > parents;
    parents << rootItem;
    // Add the necessary root nodes
    addRootNode(rootItem, pItemDataType(new stItemData("Accounts", EVT_RO_TEXT, PM_ITEM_ACCOUNTS)), pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID)), rootItem->columnCount());
    // Loop through accounts
    for (const auto &accountModel : m_pRoot->getModels())
    {
        QModelIndex accountIndex = createIndex(rootItem->childCount() - 1, 0, rootItem->child(rootItem->childCount() - 1));
        addWorkingNode(accountIndex, accountModel, PM_ITEM_ACCOUNT);

        // Loop through portfolios in the account
        for (const auto &portfolioModel : accountModel->getModels())
        {
            QModelIndex portfolioIndex = createIndex(accountIndex.row() + START_OF_WORKING_NODES, 0, getItem(accountIndex)->child(getItem(accountIndex)->childCount() - 1));
            addWorkingNode(portfolioIndex, portfolioModel, PM_ITEM_PORTFOLIO);

            // Loop through strategies in the portfolio
            for (const auto &strategyModel : portfolioModel->getModels())
            {
                QModelIndex strategyIndex = createIndex(portfolioIndex.row() + START_OF_WORKING_NODES, 0, getItem(portfolioIndex)->child(getItem(portfolioIndex)->childCount() - 1));
                addWorkingNode(strategyIndex, strategyModel, PM_ITEM_STRATEGY);
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
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_MA);
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

    addNestedNodes(parentAccount, "Parameters", pModel->getParameters(), false, item->columnCount());
    addNestedNodes(parentAccount, "Info", pModel->genericInfo(), true, item->columnCount());
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
//traverseNodes(rootItem);
    //traverseTreeView(createIndex(0, 0, rootItem));
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

const ptrGenericModelType CPortfolioConfigModel::getModelByIdex(QModelIndex index)
{
    ptrGenericModelType ret = nullptr;
    TreeItem *tmpItem = getItem(index);
    if(nullptr != tmpItem)
    {
        switch (tmpItem->data(0).id) {
        case PM_ITEM_ACCOUNT:
        {
            ret = m_pRoot->getModels().value(index.row(), nullptr);
        }
        break;
        case PM_ITEM_PORTFOLIO:
        {
            auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
            ret = account ? account->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
        }
        break;
        case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio = account ? account->getModels().value(index.parent().row() - START_OF_WORKING_NODES, nullptr) : nullptr;
            ret = portfolio ? portfolio->getModels().value(index.row() - START_OF_WORKING_NODES, nullptr) : nullptr;
        }
        break;
        default:
            break;
        }
    }
    return ret;
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

bool isValuesEqual(const QVariant &valueFromTreeView, const QVariant &valueFromModel)
{
    bool ret = false;
    switch (valueFromModel.typeId())
    {
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::Float16:
        ret = (valueFromTreeView.toString() == QString::number(valueFromModel.toDouble(), 'f', 2)) ? true : false;
        break;
    case QMetaType::Int:
        ret = (valueFromTreeView.toInt() == valueFromModel.toInt()) ? true : false;
        break;
    default:
        ret = (valueFromTreeView.toString() == valueFromModel.toString()) ? true : false;
        break;
    }
    return ret;
}


enum class NodeState
{
    None,
    Parameters,
    Info,
    Assets
};

void CPortfolioConfigModel::traverseTreeView(const QModelIndex& parentIndex)
{

    const QAbstractItemModel* model = parentIndex.model();
    if (!model) {
        return;
    }


    static quint16 node = PM_ITEM_ACCOUNTS;
    static NodeState subNode = NodeState::None;
    static QString key = "";
    static QString assetKey = "";

    TreeItem* item = getItem(parentIndex);
    static ptrGenericModelType dataModel = nullptr;

    //qDebug() << "row: " << parentIndex.row() << "col: " << parentIndex.column() << parentIndex.data(Qt::DisplayRole).toString();
    if((parentIndex.column() == 0))
    {
        if (item->data(0).id == PM_ITEM_ACCOUNTS)
        {
            qDebug() << parentIndex.data(Qt::DisplayRole).toString() << "RESET";
            node = item->data(0).id;
            subNode = NodeState::None;
            assetKey = "";
            key = "";
        }
        if (((item->data(0).id == PM_ITEM_ACCOUNT) || (item->data(0).id == PM_ITEM_PORTFOLIO) || (item->data(0).id == PM_ITEM_STRATEGY)))
        {
            dataModel = getModelByIdex(parentIndex);
            // Do something with the current index, e.g. print the data to the console
            qDebug() << parentIndex.data(Qt::DisplayRole).toString();
            node = item->data(0).id;
            subNode = NodeState::None;
        }
    }

//    qDebug() << "row: " << parentIndex.row() << "col: " << parentIndex.column();
    /* check parameters */
    if(PM_ITEM_ACCOUNTS != node)
    {

        if(parentIndex.column() == 0)
        {
            static const QHash<QString, NodeState> stateMap = {
                {"Parameters", NodeState::Parameters},
                {"Info", NodeState::Info},
                {"Assets", NodeState::Assets}
            };

            key = parentIndex.data(Qt::DisplayRole).toString();
            if (stateMap.contains(key))
            {
                    subNode = stateMap.value(key);
                    key = "";
            }
            else if(dataModel->assetList().contains(key))
            {
                    assetKey = key;
                    qDebug() << "assetKey" << assetKey;
            }

        }
        else if((parentIndex.column() == 1) && (key != ""))
        {
            if(subNode == NodeState::Parameters)
            {
                //if (dataModel->getParameters()[key].toString() == parentIndex.data(Qt::DisplayRole).toString())
                qDebug() << "Parameters" << key << (QString::number(dataModel->getParameters()[key].toDouble(), 'f', 2)) << "treeview:" << parentIndex.data(Qt::DisplayRole).toString() << isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->getParameters()[key]);
                if(!isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->getParameters()[key]))
                {
                    TreeItem * tmp = getItem(parentIndex);
                    tmp->setData(parentIndex.column(), dataModel->getParameters()[key]);
                    emit signalUpdateData(parentIndex);
                }
                key = "";
            }
            else if(subNode == NodeState::Info)
            {
                qDebug() << "Info" << key << dataModel->genericInfo()[key].toString() << "treeview:" << parentIndex.data(Qt::DisplayRole).toString() << isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->genericInfo()[key]);
                if(!isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->genericInfo()[key]))
                {
                    TreeItem * tmp = getItem(parentIndex);
                    tmp->setData(parentIndex.column(), dataModel->genericInfo()[key]);
                    emit signalUpdateData(parentIndex);
                }
                key = "";

            }
            else if((subNode == NodeState::Assets) && (assetKey != ""))
            {

                qDebug() << key << dataModel->assetList()[assetKey].toMap()[key].toString() << "treeview:" << parentIndex.data(Qt::DisplayRole).toString() << isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->assetList()[assetKey].toMap()[key]);
                if(!isValuesEqual(parentIndex.data(Qt::DisplayRole), dataModel->assetList()[assetKey].toMap()[key]))
                {
                    TreeItem * tmp = getItem(parentIndex);
                    tmp->setData(parentIndex.column(), dataModel->assetList()[assetKey].toMap()[key]);
                    emit signalUpdateData(parentIndex);
                }
                key = "";

            }
        }
    }

    int numRows = model->rowCount(parentIndex);
    int numColumns = model->columnCount(parentIndex);
    for (int row = 0; row < numRows; ++row) {
        for (int column = 0; column < numColumns; ++column) {
                QModelIndex childIndex = model->index(row, column, parentIndex);
                traverseTreeView(childIndex);
        }
    }
}

