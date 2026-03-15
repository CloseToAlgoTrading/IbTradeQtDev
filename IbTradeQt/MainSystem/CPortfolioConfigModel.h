#ifndef CPORTFOLIOCONFIGMODEL_H
#define CPORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include <QList>
#include <QTimer>
#include <QJsonObject>
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
    void addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id);
    TreeItem* addWorkingNodeContent(const bool _isModelExist, const ptrGenericModelType pModel, TreeItem* item, const QString name, const quint16 id);

    void replaceChildNode(TreeItem * parent, const ptrGenericModelType pModel/*, const quint16 id, QString modelName*/);

    void addModel(const QModelIndex& index, const QList<quint16>& ids, quint32 itemType);
    void removeModel(QModelIndex index);
    const ptrGenericModelType getModelByIdex(QModelIndex index);

    const ptrGenericModelType getTopLevelModelByIdex(QModelIndex index);
    const ModelContext getTopLevelModelByIdex2(QModelIndex index);
    quint16 nodeTypeId(const QModelIndex& index) const;

    void setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newBrokerInterface);


private:
    void addBlockViaDialog(const QString& category, const QString& jsonKey, bool isArray, quint32 itemType);
    void syncPipelineConfigAfterRemoval(quint16 itemType, const QModelIndex& index);
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
    QString m_pendingPipelineConfigPath;

public slots:
    void slotOnClickAddAccount();
    void slotOnClickAddPortfolio();
    void slotOnClickAddStrategy();
    void slotOnClickAddSelectionModel();
    void slotOnClickAddAlphaModel();
    void slotOnClickAddRebalanceModel();
    void slotOnClickAddRiskModel();
    void slotOnClickAddExecutionModel();
    void slotOnClickAddPipelineStrategy();
    void onClickRemoveNodeButton();

    void slotOnTimeoutCallback();


public slots:
    // Triggered from the "Open in Backtest Workspace" context menu entry.
    // Emits openInBacktestWorkspace with the full strategy identity.
    void slotOnClickOpenInBacktestWorkspace();

signals:
    void signalUpdateData(const QModelIndex& index);
    void signalUpdateDataAll();
    void pipelineConfigChanged(const QJsonObject& config);

    // Emitted when the user selects "Open in Backtest Workspace" on a pipeline strategy node.
    // Carries the full strategy identity so CPresenter can pre-populate the Backtest Workspace.
    void openInBacktestWorkspace(const QString& strategyId,
                                 const QString& strategyDisplayName,
                                 const QString& portfolioPath,
                                 const QJsonObject& pipelineConfig);
};

inline void CPortfolioConfigModel::setBrokerDataProvider(QSharedPointer<CBrokerDataProvider> newBrokerInterface)
{
    m_brokerInterface = newBrokerInterface;
}

#endif // CPORTFOLIOCONFIGMODEL_H
