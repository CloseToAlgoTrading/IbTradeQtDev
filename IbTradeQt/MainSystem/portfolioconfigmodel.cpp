#include "CPortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include <QVariantMap>
#include <tuple>
#include <functional>
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
    m_UpdateInfoTimer.start(5000);
}

void CPortfolioConfigModel::setupModelData()
{
    this->setupModelData(rootItem);
}

TreeItem * CPortfolioConfigModel::addRootNode(TreeItem * parent, pItemDataType name, pItemDataType value, int columnCount)
{
    TreeItem * _parent = parent;

    _parent->insertChildren(_parent->childCount(), 1, columnCount);
    _parent->child(_parent->childCount() - 1)->addData(0, name);
    _parent->child(_parent->childCount() - 1)->addData(1, value);
    return _parent->child(_parent->childCount()-1);
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
    //if (!params.empty())
    {
        auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID));
        auto _workingItem = parent;

        TreeItem *parentNode = addRootNode(_workingItem,
                                           pItemDataType(new stItemData(rootName, EVT_RO_TEXT, PM_ITEM_PARAMETERS)),
                                           EmptyRoItem,
                                           columnCount);

        TreeItem *_parent = parentNode;
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

    QList<TreeItem * > parents;
    parents << rootItem;
    // Add the necessary root nodes
    addRootNode(rootItem, pItemDataType(new stItemData("Accounts", EVT_RO_TEXT, PM_ITEM_ACCOUNTS)), pItemDataType(new stItemData(QVariant(), EVT_RO_TEXT, TVM_UNUSED_ID)), rootItem->columnCount());

    m_pRoot->setBrokerDataProvider(m_brokerInterface);
    // Loop through accounts
    for (const auto &accountModel : m_pRoot->getModels())
    {
        accountModel->setBrokerDataProvider(m_brokerInterface);
        QModelIndex accountIndex = createIndex(rootItem->childCount() - 1, 0, rootItem->child(rootItem->childCount() - 1));
        addWorkingNode(accountIndex, accountModel, PM_ITEM_ACCOUNT);

        // Loop through portfolios in the account
        for (const auto &portfolioModel : accountModel->getModels())
        {
            portfolioModel->setBrokerDataProvider(m_brokerInterface);
            auto portfolio_offset = getItem(accountIndex)->getFirstModelChildIndexCache();
            auto portfolioIndex = createIndex(accountIndex.row() + portfolio_offset, 0, getItem(accountIndex)->child(getItem(accountIndex)->childCount() - 1));
            addWorkingNode(portfolioIndex, portfolioModel, PM_ITEM_PORTFOLIO);

            // Loop through strategies in the portfolio
            for (const auto &strategyModel : portfolioModel->getModels())
            {
                strategyModel->setBrokerDataProvider(m_brokerInterface);
                auto strategy_offset = getItem(portfolioIndex)->getFirstModelChildIndexCache();
                auto correntIndex = createIndex(portfolioIndex.row() + strategy_offset, 0, getItem(portfolioIndex)->child(getItem(portfolioIndex)->childCount() - 1));
                addWorkingNode(correntIndex, strategyModel, PM_ITEM_STRATEGY);
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

        QList<quint16> Ids{PM_ITEM_STRATEGY, PM_ITEM_STRATEGIES, PM_ITEM_PORTFOLIO, PM_ITEM_ACCOUNT, PM_ITEM_SELECTION_MODEL, PM_ITEM_ALFA_MODEL, PM_ITEM_REBALANCE_MODEL, PM_ITEM_RISK_MODEL, PM_ITEM_EXECUTION_MODEL};
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
                    auto tmp_parent = itemToUpdate->parent();
                    auto tmp_id = tmp_parent->data(0).id;
                    bool parents_isActive = true;
                    while (PM_ITEM_ACCOUNTS != tmp_id) {
                        parents_isActive &= tmp_parent->data(valueIndex.column()).value.toInt() == Qt::Checked;
                        tmp_parent = tmp_parent->parent();
                        tmp_id = tmp_parent->data(0).id;
                    };
                    if(true == parents_isActive)
                    {
                        isActive ? modelToUpdate->start() : modelToUpdate->stop();
                    }
                }
            }
        };

        auto createEmptyModel = [](TreeItem *item, const QString &modelName) {
            item->setData(0, modelName);
            item->setData(1, "<Empty>");
            return static_cast<ptrGenericModelType>(nullptr);
        };


        ptrGenericModelType modelToUpdate = getTopLevelModelByIdex2(index).model;
        switch (tmpItem->data(0).id) {
        case PM_ITEM_SELECTION_MODEL:
            modelToUpdate = modelToUpdate->getSelectionModel() ? modelToUpdate->getSelectionModel() : createEmptyModel(tmpItem, "Selection Model");
            break;
        case PM_ITEM_ALFA_MODEL:
            modelToUpdate = modelToUpdate->getAlphaModel() ? modelToUpdate->getAlphaModel() : createEmptyModel(tmpItem, "Alpha Model");
            break;
        case PM_ITEM_REBALANCE_MODEL:
            modelToUpdate = modelToUpdate->getRebalanceModel() ? modelToUpdate->getRebalanceModel() : createEmptyModel(tmpItem, "Rebalance Model");
            break;
        case PM_ITEM_RISK_MODEL:
            modelToUpdate = modelToUpdate->getRiskModel() ? modelToUpdate->getRiskModel() : createEmptyModel(tmpItem, "Risk Model");
            break;
        case PM_ITEM_EXECUTION_MODEL:
            modelToUpdate = modelToUpdate->getExecutionModel() ? modelToUpdate->getExecutionModel() : createEmptyModel(tmpItem, "Execution Model");
            break;
        }

        if(nullptr != modelToUpdate) updateModel(modelToUpdate);
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
                //model->setBrokerDataProvider(this->m_brokerInterface);
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
                    //model->setBrokerDataProvider(this->m_brokerInterface);
                    account->addModel(model);
                }
            }
            break;
        case PM_ITEM_STRATEGY:
            {
                auto account = m_pRoot->getModels().value(workingIndex.parent().row(), nullptr);
                auto portfolio_offset = item->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.row() - portfolio_offset, nullptr) : nullptr;
                if((nullptr != portfolio) && (nullptr != this->m_brokerInterface))
                {
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_MOMENTUM);
                    //model->setBrokerDataProvider(this->m_brokerInterface);
                    portfolio->addModel(model);
                }
            }
            break;
        case PM_ITEM_SELECTION_MODEL:
            {
                auto account = m_pRoot->getModels().value(workingIndex.parent().parent().row(), nullptr);
                auto portfolio_offset = item->parent()->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.parent().row() - portfolio_offset, nullptr) : nullptr;
                auto strategy_offset = item->parent()->getFirstModelChildIndexCache();
                auto strategy = portfolio ? portfolio->getModels().value(workingIndex.row() - strategy_offset, nullptr) : nullptr;
                if(nullptr != strategy){
                    if(nullptr != strategy->getSelectionModel())
                    {
                        auto _offset = getItem(workingIndex)->getFirstModelChildIndexCache();
                        removeModel(this->index(_offset,0,workingIndex));
                    }
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_SELECTION_MODEL);
                    strategy->addSelectionModel(model);
                }
            }
            break;
        case PM_ITEM_ALFA_MODEL:
        {
                auto account = m_pRoot->getModels().value(workingIndex.parent().parent().row(), nullptr);
                auto portfolio_offset = item->parent()->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.parent().row() - portfolio_offset, nullptr) : nullptr;
                auto strategy_offset = item->parent()->getFirstModelChildIndexCache();
                auto strategy = portfolio ? portfolio->getModels().value(workingIndex.row() - strategy_offset, nullptr) : nullptr;
                if(nullptr != strategy){
                    if(nullptr != strategy->getAlphaModel())
                    {
                        auto _offset = getItem(workingIndex)->getFirstModelChildIndexCache();
                        removeModel(this->index(_offset+1,0,workingIndex));
                    }
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_ALPHA_MODEL);
                    strategy->addAlphaModel(model);
                }

        }
        break;
        case PM_ITEM_REBALANCE_MODEL:
        {
                auto account = m_pRoot->getModels().value(workingIndex.parent().parent().row(), nullptr);
                auto portfolio_offset = item->parent()->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.parent().row() - portfolio_offset, nullptr) : nullptr;
                auto strategy_offset = item->parent()->getFirstModelChildIndexCache();
                auto strategy = portfolio ? portfolio->getModels().value(workingIndex.row() - strategy_offset, nullptr) : nullptr;
                if(nullptr != strategy){
                    if(nullptr != strategy->getRebalanceModel())
                    {
                        auto _offset = getItem(workingIndex)->getFirstModelChildIndexCache();
                        removeModel(this->index(_offset+2,0,workingIndex));
                    }
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_REBALANCE_MODEL);
                    strategy->addRebalanceModel(model);
                }

        }
        break;
        case PM_ITEM_RISK_MODEL:
        {
                auto account = m_pRoot->getModels().value(workingIndex.parent().parent().row(), nullptr);
                auto portfolio_offset = item->parent()->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.parent().row() - portfolio_offset, nullptr) : nullptr;
                auto strategy_offset = item->parent()->getFirstModelChildIndexCache();
                auto strategy = portfolio ? portfolio->getModels().value(workingIndex.row() - strategy_offset, nullptr) : nullptr;
                if(nullptr != strategy){
                    if(nullptr != strategy->getRiskModel())
                    {
                        auto _offset = getItem(workingIndex)->getFirstModelChildIndexCache();
                        removeModel(this->index(_offset+3,0,workingIndex));
                    }
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_RISK_MODEL);
                    strategy->addRiskModel(model);
                }

        }
        break;
        case PM_ITEM_EXECUTION_MODEL:
        {
                auto account = m_pRoot->getModels().value(workingIndex.parent().parent().row(), nullptr);
                auto portfolio_offset = item->parent()->parent()->getFirstModelChildIndexCache();
                auto portfolio = account ? account->getModels().value(workingIndex.parent().row() - portfolio_offset, nullptr) : nullptr;
                auto strategy_offset = item->parent()->getFirstModelChildIndexCache();
                auto strategy = portfolio ? portfolio->getModels().value(workingIndex.row() - strategy_offset, nullptr) : nullptr;
                if(nullptr != strategy){
                    if(nullptr != strategy->getExecutionModel())
                    {
                        auto _offset = getItem(workingIndex)->getFirstModelChildIndexCache();
                        removeModel(this->index(_offset+4,0,workingIndex));
                    }
                    model = CStrategyFactory::createNewStrategy(ModelType::STRATEGY_EXECTION_MODEL);
                    strategy->addExecutionModel(model);
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

void CPortfolioConfigModel::slotOnClickAddSelectionModel()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_STRATEGY}, PM_ITEM_SELECTION_MODEL);
    }
}

void CPortfolioConfigModel::slotOnClickAddAlphaModel()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_STRATEGY}, PM_ITEM_ALFA_MODEL);
    }
}

void CPortfolioConfigModel::slotOnClickAddRebalanceModel()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_STRATEGY}, PM_ITEM_REBALANCE_MODEL);
    }
}

void CPortfolioConfigModel::slotOnClickAddRiskModel()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_STRATEGY}, PM_ITEM_RISK_MODEL);
    }
}

void CPortfolioConfigModel::slotOnClickAddExecutionModel()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        addModel(selectionModel->currentIndex(), {PM_ITEM_STRATEGY}, PM_ITEM_EXECUTION_MODEL);
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

TreeItem* CPortfolioConfigModel::addWorkingNodeContent(const bool _isModelExist, const ptrGenericModelType pModel, TreeItem* item, const QString name, const quint16 id)
{
    TreeItem *parent = nullptr;
    if(item != nullptr)
    {
        auto _vType = _isModelExist ? EVT_RO_TEXT : EVT_TEXT;
        auto _name = pModel == nullptr ? name : pModel->getName();
        auto secondIdem = _isModelExist ?
                              pItemDataType(new stItemData("<empty>", EVT_RO_TEXT, TVM_UNUSED_ID)) :
                              pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, id + PT_ITEM_ACTIVATION));
        parent = addRootNode(item,
                             pItemDataType(new stItemData(_name, _vType, id)),
                             secondIdem,
                             item->columnCount());
        if(nullptr != pModel)
        {
            addNestedNodes(parent, "Parameters", pModel->getParameters(), false, item->columnCount());
            addNestedNodes(parent, "Info", pModel->genericInfo(), true, item->columnCount());
            addNestedNodes(parent, "Assets", pModel->assetList(), true, item->columnCount());
        }
    }
    return parent;
}
void CPortfolioConfigModel::addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id, QString modelName)
{
    TreeItem * item = getItem(index);
    TreeItem * existingChild = nullptr;


    bool _isModelExist = (pModel != nullptr) &&
                         ((pModel->modelType() == ModelType::STRATEGY_SELECTION_MODEL)
                          || (pModel->modelType() == ModelType::STRATEGY_ALPHA_MODEL)
                          || (pModel->modelType() == ModelType::STRATEGY_REBALANCE_MODEL)
                          || (pModel->modelType() == ModelType::STRATEGY_RISK_MODEL)
                          || (pModel->modelType() == ModelType::STRATEGY_EXECTION_MODEL));

    if (_isModelExist) {
      // Check if a child with the given id already exists
      for (int i = 0; i < item->childCount(); ++i) {
        if (item->child(i)->data(0).id == id) {
                existingChild = item->child(i);
                break;
        }
      }
    }

    if (existingChild) {
      // Replace the existing child's data

        beginInsertRows(createIndex(0,0,existingChild),0,0);
        //replaceChildNode(existingChild, pModel, id, modelName);
        replaceChildNode(existingChild, pModel);
        endInsertRows();

        QModelIndex topLeft = createIndex(index.row(), 0, existingChild);
        QModelIndex bottomRight = createIndex(index.row(), columnCount(index) - 1, existingChild);
        emit dataChanged(topLeft, bottomRight);
        emit layoutChanged();
    }
    else
    {
//       Add a new child node
        beginInsertRows(index,item->childCount(),item->childCount());
        TreeItem * parent;
        if(pModel != nullptr)
        {
          parent = addWorkingNodeContent(_isModelExist, pModel, item, pModel->getName(), id);
          if(PM_ITEM_STRATEGY == id)
          {
                using ModelGetter = std::function<ptrGenericModelType(ptrGenericModelType)>;
                std::array<std::tuple<ModelGetter, int, std::string>, 5> modelInfos = {
                    std::make_tuple(ModelGetter([](ptrGenericModelType model) { return model->getSelectionModel(); }), PM_ITEM_SELECTION_MODEL, "Selection Model"),
                    std::make_tuple(ModelGetter([](ptrGenericModelType model) { return model->getAlphaModel(); }), PM_ITEM_ALFA_MODEL, "Alpha Model"),
                    std::make_tuple(ModelGetter([](ptrGenericModelType model) { return model->getRebalanceModel(); }), PM_ITEM_REBALANCE_MODEL, "Rebalance Model"),
                    std::make_tuple(ModelGetter([](ptrGenericModelType model) { return model->getRiskModel(); }), PM_ITEM_RISK_MODEL, "Risk Model"),
                    std::make_tuple(ModelGetter([](ptrGenericModelType model) { return model->getExecutionModel(); }), PM_ITEM_EXECUTION_MODEL, "Execution Model")
                };

                for (const auto& [getModel, modelItem, modelName] : modelInfos) {
                        if(nullptr != pModel)
                            pModel->setBrokerDataProvider(m_brokerInterface);
                        (void)addWorkingNodeContent(true, getModel(pModel), parent, modelName.c_str(), modelItem);

                }
          }
        }
//        else
//        {
//          parent = RootNode(item,
//                              pItemDataType(new stItemData(modelName, EVT_RO_TEXT, id)),
//                              pItemDataType(new stItemData("<empty>", EVT_RO_TEXT, id)),
//                              item->columnCount());
//        }
        endInsertRows();

        QModelIndex newIndex = createIndex(index.row(), 0, item);
        emit signalUpdateData(newIndex);
    }
}

// Define the replaceChildNode function to handle replacing the child's data
void CPortfolioConfigModel::replaceChildNode(TreeItem * parent, const ptrGenericModelType pModel/*, const quint16 id, QString modelName*/)
{
    // Replace the data of the existingChild with the new data
    // This is just a basic example, you might need to adjust it based on your needs
    parent->setData(0, pModel->getName());
    parent->setData(1, QVariant());
    // Add any other necessary modifications here
    addNestedNodes(parent, "Parameters", pModel->getParameters(), false, parent->columnCount());
    addNestedNodes(parent, "Info", pModel->genericInfo(), true, parent->columnCount());
    addNestedNodes(parent, "Assets", pModel->assetList(), true, parent->columnCount());
}


void CPortfolioConfigModel::onClickRemoveNodeButton()
{
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();

    if (selectionModel->hasSelection()) {
        QModelIndex index = selectionModel->currentIndex(); // Assumes single selection mode
        if(PM_ITEM_ACCOUNTS != getItem(index)->data(0).id)
        {
            QList<quint16> Ids{PM_ITEM_STRATEGY, PM_ITEM_STRATEGIES, PM_ITEM_PORTFOLIO, PM_ITEM_ACCOUNT, PM_ITEM_SELECTION_MODEL, PM_ITEM_ALFA_MODEL, PM_ITEM_REBALANCE_MODEL, PM_ITEM_RISK_MODEL, PM_ITEM_EXECUTION_MODEL};
            index = findWorkingNode(index, Ids);

            removeModel(index);
        }
    }
}

void CPortfolioConfigModel::slotOnTimeoutCallback()
{
    traverseTreeView(createIndex(0, 0, rootItem));
}


void CPortfolioConfigModel::removeModel(QModelIndex index)
{
    TreeItem *tmpItem = getItem(index);

    // Utilize the new function to simplify getting the model and parent
    ModelContext context = getTopLevelModelByIdex2(index);
    ptrGenericModelType modelToRemove = context.model;
    ptrGenericModelType parentModel = context.parentModel;

    if (modelToRemove == nullptr) {
        return; // Early return if there's nothing to remove
    }

    // Determine the type of item and proceed with removal
    switch (tmpItem->data(0).id)
    {
    case PM_ITEM_ACCOUNT:
        m_pRoot->removeModel(modelToRemove);
        break;
    case PM_ITEM_PORTFOLIO:
        if (parentModel)
            parentModel->removeModel(modelToRemove);
        break;
    case PM_ITEM_STRATEGY:
        if (parentModel)
            parentModel->removeModel(modelToRemove);
        break;
    case PM_ITEM_SELECTION_MODEL:
        modelToRemove->removeSelectionModel();
        break;
    case PM_ITEM_ALFA_MODEL:
        modelToRemove->removeAlphaModel();
        break;
    case PM_ITEM_REBALANCE_MODEL:
        modelToRemove->removeRebalanceModel();
        break;
    case PM_ITEM_RISK_MODEL:
        modelToRemove->removeRiskModel();
        break;
    case PM_ITEM_EXECUTION_MODEL:
        modelToRemove->removeExecutionModel();
        break;
    default:
        break;
    }


    if((PM_ITEM_SELECTION_MODEL == tmpItem->data(0).id)
        || (PM_ITEM_ALFA_MODEL == tmpItem->data(0).id)
        || (PM_ITEM_RISK_MODEL == tmpItem->data(0).id)
        || (PM_ITEM_REBALANCE_MODEL == tmpItem->data(0).id)
        || (PM_ITEM_EXECUTION_MODEL == tmpItem->data(0).id))
    {
        int childCount = tmpItem->childCount();
        if(childCount > 0)
        {
                // Notify the view that you're about to remove rows (child nodes)
                beginRemoveRows(index, 0, childCount - 1);
                // Remove all child nodes of tmpItem
                tmpItem->removeChildren(0, childCount);
                // Notify the view that you've finished removing rows
                endRemoveRows();
                emit signalUpdateData(createIndex(0, 0, rootItem));

                QModelIndex topLeft = createIndex(index.row(), 0, index.internalPointer());
                QModelIndex bottomRight = createIndex(index.row(), columnCount(index) - 1, index.internalPointer());

                emit dataChanged(topLeft, bottomRight);
        }
    }
    else
    {
        // A. Validate Index and Item Consistency
        if(index.row() != tmpItem->childNumber()) {
                qDebug() << "Mismatch: index.row() =" << index.row() << ", tmpItem->childNumber() =" << tmpItem->childNumber();
        }
        beginRemoveRows(index.parent(), index.row(), index.row());
        bool removed = tmpItem->parent()->removeChildren(tmpItem->childNumber(), 1);
        if(!removed) {
                qDebug() << "Removal failed";
        }
        endRemoveRows();

    }
}

const ptrGenericModelType CPortfolioConfigModel::getModelByIdex(QModelIndex index)
{

    ptrGenericModelType ret = getTopLevelModelByIdex2(index).model;
    if (ret) {
        TreeItem *tmpItem = getItem(index);
        if (tmpItem) {
            switch (tmpItem->data(0).id)
            {
            case PM_ITEM_SELECTION_MODEL:
                ret = ret->getSelectionModel();
            break;
            case PM_ITEM_ALFA_MODEL:
                ret = ret->getAlphaModel();
            break;
            case PM_ITEM_REBALANCE_MODEL:
                ret = ret->getRebalanceModel();
            break;
            case PM_ITEM_RISK_MODEL:
                ret = ret->getRiskModel();
            break;
            case PM_ITEM_EXECUTION_MODEL:
                ret = ret->getExecutionModel();
            break;
            default:
            // Do nothing for other cases
            break;
            }
        }
    }
    return ret;
}


const ModelContext CPortfolioConfigModel::getTopLevelModelByIdex2(QModelIndex index)
{
    ptrGenericModelType parentModel = nullptr;
    ptrGenericModelType retModel = nullptr;
    TreeItem *tmpItem = getItem(index);
    if(nullptr != tmpItem)
    {
        switch (tmpItem->data(0).id) {
        case PM_ITEM_ACCOUNT:
        {
            retModel = m_pRoot->getModels().value(index.row(), nullptr);
        }
        break;
        case PM_ITEM_PORTFOLIO:
        {
            auto account = m_pRoot->getModels().value(index.parent().row(), nullptr);
            auto portfolio_offset = tmpItem->parent()->getFirstModelChildIndexCache();
            retModel = account ? account->getModels().value(index.row() - portfolio_offset, nullptr) : nullptr;
            parentModel = account;
        }
        break;
        case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            auto portfolio = account ? account->getModels().value(index.parent().row() - portfolio_offset, nullptr) : nullptr;
            auto strategy_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            retModel = portfolio ? portfolio->getModels().value(index.row() - strategy_offset, nullptr) : nullptr;
            parentModel = portfolio;
        }
        break;
        case PM_ITEM_SELECTION_MODEL:
        case PM_ITEM_ALFA_MODEL:
        case PM_ITEM_REBALANCE_MODEL:
        case PM_ITEM_RISK_MODEL:
        case PM_ITEM_EXECUTION_MODEL:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().parent().row(), nullptr);
            auto portfolio_offset = tmpItem->parent()->parent()->parent()->getFirstModelChildIndexCache();
            auto portfolio = account ? account->getModels().value(index.parent().parent().row() - portfolio_offset, nullptr) : nullptr;
            auto strategy_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            retModel = portfolio ? portfolio->getModels().value(index.parent().row() - strategy_offset, nullptr) : nullptr;
            parentModel = portfolio;
        }
        break;
        default:
            break;
        }
    }
    return ModelContext(retModel, parentModel);
}


const ptrGenericModelType  CPortfolioConfigModel::getTopLevelModelByIdex(QModelIndex index)
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
            auto portfolio_offset = tmpItem->parent()->getFirstModelChildIndexCache();
            ret = account ? account->getModels().value(index.row() - portfolio_offset, nullptr) : nullptr;
        }
        break;
        case PM_ITEM_STRATEGY:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().row(), nullptr);
            auto portfolio_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            auto portfolio = account ? account->getModels().value(index.parent().row() - portfolio_offset, nullptr) : nullptr;
            auto strategy_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            ret = portfolio ? portfolio->getModels().value(index.row() - strategy_offset, nullptr) : nullptr;
        }
        break;
        case PM_ITEM_SELECTION_MODEL:
        case PM_ITEM_ALFA_MODEL:
        case PM_ITEM_REBALANCE_MODEL:
        case PM_ITEM_RISK_MODEL:
        case PM_ITEM_EXECUTION_MODEL:
        {
            auto account = m_pRoot->getModels().value(index.parent().parent().parent().row(), nullptr);
            auto portfolio_offset = tmpItem->parent()->parent()->parent()->getFirstModelChildIndexCache();
            auto portfolio = account ? account->getModels().value(index.parent().parent().row() - portfolio_offset, nullptr) : nullptr;
            auto strategy_offset = tmpItem->parent()->parent()->getFirstModelChildIndexCache();
            ret = portfolio ? portfolio->getModels().value(index.parent().row() - strategy_offset, nullptr) : nullptr;
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

    if((parentIndex.column() == 0))
    {
        if (item->data(0).id == PM_ITEM_ACCOUNTS)
        {
            node = item->data(0).id;
            subNode = NodeState::None;
            assetKey = "";
            key = "";
        }
        if (((item->data(0).id == PM_ITEM_ACCOUNT) || (item->data(0).id == PM_ITEM_PORTFOLIO) || (item->data(0).id == PM_ITEM_STRATEGY)
             || (item->data(0).id == PM_ITEM_REBALANCE_MODEL)|| (item->data(0).id == PM_ITEM_ALFA_MODEL)
             || (item->data(0).id == PM_ITEM_EXECUTION_MODEL) || (item->data(0).id == PM_ITEM_SELECTION_MODEL)
             || (item->data(0).id == PM_ITEM_RISK_MODEL)))
        {
            dataModel = getModelByIdex(parentIndex);
            // Do something with the current index, e.g. print the data to the console
            node = item->data(0).id;
            subNode = NodeState::None;
        }
    }

    /* check parameters */
    if((nullptr != dataModel) && (PM_ITEM_ACCOUNTS != node))
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
                if(key == "Parameters")
                {
                    synchronizeModelParameters(parentIndex, dataModel->getParameters());
                }
                else if(key == "Info")
                {
                    synchronizeModelParameters(parentIndex, dataModel->genericInfo());
                }
                else if(key == "Assets")
                {
                    synchronizeModelParameters(parentIndex, dataModel->assetList());
                }
                subNode = stateMap.value(key);
                key = "";

            }
            else if(dataModel->assetList().contains(key))
            {
                    assetKey = key;
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


void CPortfolioConfigModel::updateNodeIfChanged(const QModelIndex& index, const QVariant& valueFromModel)
{
    if (!isValuesEqual(index.data(Qt::DisplayRole), valueFromModel))
    {
        TreeItem* tmp = getItem(index);
        tmp->setData(index.column(), valueFromModel);
        emit signalUpdateData(index);
    }
}

void CPortfolioConfigModel::synchronizeModelParameters(const QModelIndex& parentIndex, const QVariantMap& modelParameters)
{
    QStringList keysFromModel = modelParameters.keys();

    int treeViewRowCount = rowCount(parentIndex);

    // Add or update rows in the tree view to match the model parameters
    for (int i = 0; i < keysFromModel.size(); ++i)
    {
        QModelIndex childIndex = index(i, 0, parentIndex);
        TreeItem* childItem = getItem(childIndex);

        // Check if the tree view node key matches the model parameter key
        if (i < treeViewRowCount)
        {
                //QModelIndex valueIndex = index(i, 1, parentIndex);
                QVariant treeViewValue = childItem->data(1).value;
                QVariant modelValue = modelParameters[keysFromModel[i]];

                // If the tree view value doesn't match the model value, update the tree view value
                if (keysFromModel[i] != childItem->data(0).value.toString())
                {
                    childItem->setData(0, keysFromModel[i]);
                    emit signalUpdateData(index(i, 0, parentIndex));
                }
                if (!isValuesEqual(treeViewValue, modelValue))
                {
                    childItem->setData(1, modelValue);
                    emit signalUpdateData(index(i, 1, parentIndex));
                }



                // If the value is a QVariantMap, recursively synchronize its children
                if (modelValue.typeId() == QMetaType::QVariantMap)
                {
                synchronizeModelParameters(childIndex, modelValue.toMap());
                }
        }
        else
        {
                // Insert a new row for the new model parameter
                beginInsertRows(parentIndex, i, i);
                addDataToNode(getItem(parentIndex), keysFromModel[i], modelParameters[keysFromModel[i]], modelParameters[keysFromModel[i]].typeId(), true, columnCount(parentIndex));
                endInsertRows();

                // If the value is a QVariantMap, recursively synchronize its children
                if (modelParameters[keysFromModel[i]].typeId() == QMetaType::QVariantMap)
                {
                QModelIndex newNodeIndex = index(i, 0, parentIndex);
                synchronizeModelParameters(newNodeIndex, modelParameters[keysFromModel[i]].toMap());
                }
        }
    }

    // Remove extra rows in the tree view if they don't exist in the model parameters
    while (treeViewRowCount > keysFromModel.size())
    {
        beginRemoveRows(parentIndex, treeViewRowCount - 1, treeViewRowCount - 1);
        getItem(parentIndex)->removeChildren(treeViewRowCount - 1, 1);
        endRemoveRows();
        treeViewRowCount--;
    }
}
