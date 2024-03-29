#ifndef CPORTFOLIOCONFIGMODEL_H
#define CPORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include <QList>
#include <QTimer>
#include "cgenericmodelApi.h"

class CBasicRoot;

struct ModelContext {
    ptrGenericModelType model;
    ptrGenericModelType parentModel;
    // Constructor for convenience
    ModelContext(ptrGenericModelType m = nullptr, ptrGenericModelType p = nullptr) : model(m), parentModel(p) {}
};

class CPortfolioConfigModel : public CTreeViewCustomModel
{
    Q_OBJECT
public:
    CPortfolioConfigModel(QTreeView *treeView, CBasicRoot *pRoot, QObject *parent);

    void setupModelData();
    void setupModelData(TreeItem * rootItem);
    //void addGenericModelToNodes(ptrGenericModelType inputModel, QModelIndex correntIndex);

    QModelIndex findWorkingNode(QModelIndex index, const QList<quint16> & Ids);
    void addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id, QString modelName = "");
    TreeItem* addWorkingNodeContent(const bool _isModelExist, const ptrGenericModelType pModel, TreeItem* item, const QString name, const quint16 id);

    void replaceChildNode(TreeItem * parent, const ptrGenericModelType pModel/*, const quint16 id, QString modelName*/);

    void addModel(const QModelIndex& index, const QList<quint16>& ids, quint32 itemType);
    void removeModel(QModelIndex index);
    const ptrGenericModelType getModelByIdex(QModelIndex index);

    const ptrGenericModelType getTopLevelModelByIdex(QModelIndex index);
    const ModelContext getTopLevelModelByIdex2(QModelIndex index);

    void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newBrokerInterface);




private:
    void traverseNodes(TreeItem *node);
    void processNode(TreeItem *node);
    void traverseTreeView(const QModelIndex &parentIndex);
    void updateNodeIfChanged(const QModelIndex& index, const QVariant& valueFromModel);
    void synchronizeModelParameters(const QModelIndex& parentIndex, const QVariantMap& modelParameters);


    void addDataToNode(TreeItem *parent, const QString &key, const QVariant &value, int typeId, bool readOnly, int columnCount);
    void addNestedNodes(TreeItem *parent, const QString &rootName, const QVariantMap &params, bool readOnly, int columnCount);
    TreeItem * addRootNode(TreeItem * parent, pItemDataType name, pItemDataType value, int columnCount);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;


private:
    CBasicRoot *m_pRoot;
    QSharedPointer<CBrokerDataProvider> m_brokerInterface;
    QTimer m_UpdateInfoTimer;

public slots:
    void slotOnClickAddAccount();
    void slotOnClickAddPortfolio();
    void slotOnClickAddStrategy();
    void slotOnClickAddSelectionModel();
    void slotOnClickAddAlphaModel();
    void slotOnClickAddRebalanceModel();
    void slotOnClickAddRiskModel();
    void slotOnClickAddExecutionModel();
    void onClickRemoveNodeButton();

    void slotOnTimeoutCallback();


signals:
    void signalUpdateData(const QModelIndex& index);
    void signalUpdateDataAll();
};

inline void CPortfolioConfigModel::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newBrokerInterface)
{
    m_brokerInterface = newBrokerInterface;
}

#endif // CPORTFOLIOCONFIGMODEL_H
