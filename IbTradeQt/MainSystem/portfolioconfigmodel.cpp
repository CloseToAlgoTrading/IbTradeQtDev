#include "PortfolioConfigModel.h"
#include "TreeItemDataTypesDef.h"
#include "PortfolioModelDefines.h"

#include "caccount.h"
#include <QVariantMap>

#include "cteststrategy.h"
#include "cbasicportfolio.h"
#include "cbasicaccount.h"

PortfolioConfigModel::PortfolioConfigModel(QObject *parent)
    : m_Item1()
    , m_Item2()
    , m_Root()
{
    Q_UNUSED(parent);
    CTestStrategy *pS1 = new CTestStrategy();
    pS1->setName("S1");
    CTestStrategy *pS2 = new CTestStrategy();
    pS2->setName("S2");
    CTestStrategy *pS3 = new CTestStrategy();
    pS3->setName("S3");
    CBasicPortfolio *pP1 = new CBasicPortfolio();
    pP1->setName("P1");
    CBasicPortfolio *pP2 = new CBasicPortfolio();
    pP2->setName("P2");
    CBasicAccount *pA1 = new CBasicAccount();
    pA1->setName("A1");
    CBasicAccount *pA2 = new CBasicAccount();
    pA2->setName("A2");

    pP1->addModel(static_cast<ptrGenericModelType>(pS1));
    pP1->addModel(static_cast<ptrGenericModelType>(pS2));

    pP2->addModel(static_cast<ptrGenericModelType>(pS3));

    pA1->addModel(static_cast<ptrGenericModelType>(pP1));
    pA1->addModel(static_cast<ptrGenericModelType>(pP2));

    m_Root.addModel(static_cast<ptrGenericModelType>(pA1));
    m_Root.addModel(static_cast<ptrGenericModelType>(pA2));
}

TreeItem* addRootNode(TreeItem *parent, pItemDataType name, pItemDataType value, int columnCount)
{
    TreeItem *_parent = parent;

    _parent->insertChildren(_parent->childCount(), 1, columnCount);
    _parent->child(_parent->childCount() - 1)->addData(0, name);
    _parent->child(_parent->childCount() - 1)->addData(1, value);
    return parent;
}

void addParameters(TreeItem *parent, const QVariantMap params, int columnCount)
{
    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVET_RO_TEXT, TVM_UNUSED_ID));
    if(!params.empty())
    {
        //Create account paramters
        TreeItem *parentParameters = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, PM_ITEM_PARAMETERS)),
                                       EmptyRoItem,
                                       columnCount);

        TreeItem *_parent = parentParameters->child(parentParameters->childCount() - 1);
        for (auto i = params.begin(); i != params.end(); ++i)
        {
            _parent->insertChildren(_parent->childCount(), 1, columnCount);
            _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData(i.key(), EVET_RO_TEXT, PM_ITEM_PARAMETER)));
            _parent->child(_parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(i.value().toInt(), EVET_RO_TEXT, TVM_UNUSED_ID)));
        }
    }
}

void PortfolioConfigModel::setupModelData(TreeItem *rootItem)
{
    CAccount myMap;
    auto EmptyRoItem = pItemDataType(new stItemData(QVariant(), EVET_RO_TEXT, TVM_UNUSED_ID));

    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, TVM_UNUSED_ID)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, TVM_UNUSED_ID)));

    QList<TreeItem *> parents;
    parents << rootItem;

    //Create an Accounts
    TreeItem *parent = addRootNode(parents.last(),
                                   pItemDataType(new stItemData("Accounts", EVET_RO_TEXT, PM_ITEM_ACCOUNTS)),
                                   EmptyRoItem,
                                   rootItem->columnCount());

    //*+++++++++++++++++++++++++
    for (auto&& account : this->m_Root.getModels()) {
        TreeItem *parentAccount = addRootNode(parent->child(parent->childCount() - 1),
                                       pItemDataType(new stItemData(account->getName(), EVET_RO_TEXT, PM_ITEM_ACCOUNT)),
                                       EmptyRoItem,
                                       rootItem->columnCount());

        addParameters(parentAccount, account->getParameters(), rootItem->columnCount());

        for (auto&& portfolio : account->getModels()) {
            TreeItem *parentPortfolio = addRootNode(parentAccount->child(parentAccount->childCount() - 1),
                                           pItemDataType(new stItemData(portfolio->getName(), EVET_RO_TEXT, PM_ITEM_PORTFOLIO)),
                                           EmptyRoItem,
                                           rootItem->columnCount());

            addParameters(parentPortfolio, portfolio->getParameters(), rootItem->columnCount());

            for (auto&& strategy : portfolio->getModels()) {
                TreeItem *parentStrategy = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
                                               pItemDataType(new stItemData(strategy->getName(), EVET_RO_TEXT, PM_ITEM_STRATEGY)),
                                               EmptyRoItem,
                                               rootItem->columnCount());

                addParameters(parentStrategy, strategy->getParameters(), rootItem->columnCount());
            }

        }

    }


    //++++++++++++++++++++++++++++
//    //Create an Accounts
//    TreeItem *parent = addRootNode(parents.last(),
//                                   pItemDataType(new stItemData("Accounts", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());


//    //-----> Account
//    //Create an Account
//    TreeItem *parentAccount1 = addRootNode(parent->child(parent->childCount() - 1),
//                                   pItemDataType(new stItemData("IB", EVET_RO_TEXT, 0xF000)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());


//    //Create account paramters
//    TreeItem *parentAccount1Parameters = addRootNode(parentAccount1->child(parentAccount1->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());

//    //add Parameters
//    addParameters(parentAccount1Parameters->child(parentAccount1Parameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());

//    //-----> Portfolios
//    //Create Portfolios Node
//    TreeItem *parentPortfolios = addRootNode(parentAccount1->child(parentAccount1->childCount() - 1),
//                                   pItemDataType(new stItemData("Portfolios", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentPortfoliosParameters = addRootNode(parentPortfolios->child(parentPortfolios->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Add Portfolio Parameters
//    addParameters(parentPortfoliosParameters->child(parentPortfoliosParameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());


//    //-----> Portfolio 1
//    //Create Portfolio Node
//    TreeItem *parentPortfolio = addRootNode(parentPortfolios->child(parentPortfolios->childCount() - 1),
//                                   pItemDataType(new stItemData("Portfolio", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   pItemDataType(new stItemData("Port1", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentPortfolioParameters = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());

//    //Add Portfolio Parameters
//    addParameters(parentPortfolioParameters->child(parentPortfolioParameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());


//    //-----> Strategies
//    TreeItem *parentStrategies = addRootNode(parentPortfolio->child(parentPortfolio->childCount() - 1),
//                                   pItemDataType(new stItemData("Strategies", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentStrategiesParameters = addRootNode(parentStrategies->child(parentStrategies->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Add Portfolio Parameters
//    addParameters(parentStrategiesParameters->child(parentStrategiesParameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());


//    //-----> Strategies 1
//    TreeItem *parentStrategy1 = addRootNode(parentStrategies->child(parentStrategies->childCount() - 1),
//                                   pItemDataType(new stItemData("Strategy", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   pItemDataType(new stItemData("St_1", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentStrategy1Parameters = addRootNode(parentStrategy1->child(parentStrategy1->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Add Portfolio Parameters
//    addParameters(parentStrategy1Parameters->child(parentStrategy1Parameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());

//    //-----> Strategies 2
//    TreeItem *parentStrategy2 = addRootNode(parentStrategies->child(parentStrategies->childCount() - 1),
//                                   pItemDataType(new stItemData("Strategy", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   pItemDataType(new stItemData("St_1", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentStrategy2Parameters = addRootNode(parentStrategy2->child(parentStrategy2->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());
//    //Add Portfolio Parameters
//    addParameters(parentStrategy2Parameters->child(parentStrategy2Parameters->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());


//    //Portfolio 2
//    //Create Portfolio Node
//    TreeItem *parentPortfolio2 = addRootNode(parentPortfolios->child(parentPortfolios->childCount() - 1),
//                                   pItemDataType(new stItemData("Portfolio", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   pItemDataType(new stItemData("Port2", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   rootItem->columnCount());
//    //Create Portfolios parameters Mone
//    TreeItem *parentPortfolioParameters2 = addRootNode(parentPortfolio2->child(parentPortfolio2->childCount() - 1),
//                                   pItemDataType(new stItemData("Parameters", EVET_RO_TEXT, TVM_UNUSED_ID)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());

//    //Add Portfolio Parameters
//    addParameters(parentPortfolioParameters2->child(parentPortfolioParameters2->childCount() - 1), myMap.parametersMap(), rootItem->columnCount());


//    //Create an Account
//    TreeItem *parentAccount2 = addRootNode(parent->child(parent->childCount() - 1),
//                                   pItemDataType(new stItemData("IB2", EVET_RO_TEXT, 0xF000)),
//                                   EmptyRoItem,
//                                   rootItem->columnCount());

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
//            if((S_DATA_ID_LOG_LEVEL_ALL <= itemData.id) && (S_DATA_ID_LOG_LEVEL_DEBUG >= itemData.id))
//            {
//                emit signalEditLogSettingsCompleted();
//            }
//            else if(S_DATA_ID_SERVER_ADDRESS == itemData.id)
//            {
//                signalEditServerAddresCompleted(itemData.value.toString());
//            }
//            else if(S_DATA_ID_SERVER_PORT == itemData.id)
//            {
//                signalEditServerPortCompleted(itemData.value.toInt());
//            }

        }
    }
}
