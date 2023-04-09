#ifndef CPORTFOLIOCONFIGMODEL_H
#define CPORTFOLIOCONFIGMODEL_H

#include "ctreeviewcustommodel.h"
#include "cbasicroot.h"
#include <QList>
#include <QTimer>


class CPortfolioConfigModel : public CTreeViewCustomModel
{
    Q_OBJECT
public:
    CPortfolioConfigModel(QTreeView *treeView, CBasicRoot *pRoot, QObject *parent);

    void setupModelData(TreeItem * rootItem);
    QModelIndex findWorkingNode(QModelIndex index, const QList<quint16> & Ids);
    void addWorkingNode(QModelIndex index, const ptrGenericModelType pModel, const quint16 id);

    void addModel(const QModelIndex& index, const QList<quint16>& ids, quint32 itemType);
    void removeModel(QModelIndex index);

    void setBrokerInterface(QSharedPointer<IBrokerAPI> newBrokerInterface);




private:
    void traverseNodes(TreeItem *node);
    void processNode(TreeItem *node);

    void addDataToNode(TreeItem *parent, const QString &key, const QVariant &value, int typeId, bool readOnly, int columnCount);
    void addNestedNodes(TreeItem *parent, const QString &rootName, const QVariantMap &params, bool readOnly, int columnCount);
    TreeItem * addRootNode(TreeItem * parent, pItemDataType name, pItemDataType value, int columnCount);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;


private:
    CBasicRoot *m_pRoot;
    QSharedPointer<IBrokerAPI> m_brokerInterface;
    QTimer m_UpdateInfoTimer;

public slots:
    void slotOnClickAddAccount();
    void slotOnClickAddPortfolio();
    void slotOnClickAddStrategy();
    void onClickRemoveNodeButton();

    void slotOnTimeoutCallback();


signals:
    void signalUpdateData(const QModelIndex& index);
    void signalUpdateDataAll();
};

inline void CPortfolioConfigModel::setBrokerInterface(QSharedPointer<IBrokerAPI> newBrokerInterface)
{
    m_brokerInterface = newBrokerInterface;
}

#endif // CPORTFOLIOCONFIGMODEL_H
