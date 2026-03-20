#ifndef CSETTINSMODELDATA_H
#define CSETTINSMODELDATA_H

#include "treeitem.h"
#include "ctreeviewdatamodel.h"
#include <QSharedPointer>
#include <QObject>
#include <QModelIndex>
#include <QTreeView>
#include "ctreeviewcustommodel.h"

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
    S_DATA_ID_MODEL_STORE_BACKEND,
    S_DATA_ID_MODEL_STORE_SQLITE_PATH,
    S_DATA_ID_PG_HOST,
    S_DATA_ID_PG_PORT,
    S_DATA_ID_PG_DATABASE,
    S_DATA_ID_PG_USER,
    S_DATA_ID_PG_PASSWORD,
    S_DATA_ID_APP_DATA_PATH,
    S_DATA_ID_BACKTEST_PATH,
    S_DATA_ID_COUNT
};

class CSettinsModelData: public CTreeViewCustomModel
{
    Q_OBJECT
public:
    CSettinsModelData(QTreeView *treeView, QObject *parent);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setupModelData(TreeItem * rootItem);

    void setLoggerSettings(const QVector<bool> _levels);
    const QVector<bool> getLoggerSettings();
    void setServerSettings(const QString &_addr, const quint16 &_port);

    static quint8 getMaskFromLoggerSettings(const QVector<bool> &_levels);
    static void updateLoggerSettingsArray(quint8 _mask, QVector<bool> & _levels);

    void createSettingsView();


public slots:
    virtual void dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param) final;
    void slotEditSettingLogSettingsComlited();

signals:
    void signalEditLogSettingsCompleted();
    void signalEditServerAddresCompleted(const QString & _addres);
    void signalEditServerPortCompleted(const qint32 & _port);


private:
    void onSettingsDoubleClicked(const QModelIndex &index);

    void persistStorageSettingsFromUi();
    bool storageReconfigurationAllowed() const;
    /** Show SQLite path vs PostgreSQL rows based on ModelStore backend. */
    void applyStorageVisibilityForBackend();

    pItemDataType m_pServerAddress;
    pItemDataType m_pServerPort;
    QVector<pItemDataType> m_plogLevel;

    pItemDataType m_pModelStoreBackend;
    pItemDataType m_pModelStoreSqlitePath;
    pItemDataType m_pPgHost;
    pItemDataType m_pPgPort;
    pItemDataType m_pPgDatabase;
    pItemDataType m_pPgUser;
    pItemDataType m_pPgPassword;
    pItemDataType m_pAppDataPath;
    pItemDataType m_pBacktestPath;

};

#endif // CSETTINSMODELDATA_H
