#ifndef CSETTINSMODELDATA_H
#define CSETTINSMODELDATA_H

#include "treeitem.h"
#include "ctreeviewdatamodel.h"
#include <QSharedPointer>
#include <QObject>
#include <QModelIndex>
#include <QTreeView>
#include "CTreeViewCustomModel.h"

enum S_LOG_LEVEL{
    S_INDEX_LOG_LEVEL_ALL = 0,
    S_INDEX_LOG_LEVEL_FATAL,
    S_INDEX_LOG_LEVEL_ERROR,
    S_INDEX_LOG_LEVEL_WARNING,
    S_INDEX_LOG_LEVEL_INFO,
    S_INDEX_LOG_LEVEL_DEBUG,
    S_LOG_LEVEL_COUNT
};

enum S_DATA_IDS{
    S_DATA_ID_UNSET = 0,
    S_DATA_ID_LOG_LEVEL_ALL,
    S_DATA_ID_LOG_LEVEL_FATAL,
    S_DATA_ID_LOG_LEVEL_ERROR,
    S_DATA_ID_LOG_LEVEL_WARNING,
    S_DATA_ID_LOG_LEVEL_INFO,
    S_DATA_ID_LOG_LEVEL_DEBUG,
    S_DATA_ID_SERVER_ADDRESS,
    S_DATA_ID_SERVER_PORT,
    S_DATA_ID_COUNT
};

class CSettinsModelData: public CTreeViewCustomModel
{
    Q_OBJECT
public:
    CSettinsModelData(QTreeView *treeView, QObject *parent);

    void setupModelData(TreeItem * rootItem);

    void setLoggerSettings(const QVector<bool> _levels);
    const QVector<bool> getLoggerSettings();
    void setServerSettings(const QString &_addr, const quint16 &_port);

    static quint8 getMaskFromLoggerSettings(const QVector<bool> &_levels);
    static void updateLoggerSettingsArray(quint8 _mask, QVector<bool> & _levels);

public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;

signals:
    void signalEditLogSettingsCompleted();
    void signalEditServerAddresCompleted(const QString & _addres);
    void signalEditServerPortCompleted(const qint32 & _port);


private:
    pItemDataType m_pServerAddress;
    pItemDataType m_pServerPort;
    QVector<pItemDataType> m_plogLevel;


};

#endif // CSETTINSMODELDATA_H
