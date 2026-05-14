#include "csettinsmodeldata.h"
#include "cmainmodel.h"
#include "GlobalDef.h"
#include "MyLogger.h"
#include "NHelper.h"
#include "StorageConfig.h"
#include <QAbstractItemView>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

CSettinsModelData::CSettinsModelData(QTreeView *treeView, QObject *parent)
    : CTreeViewCustomModel(treeView, parent),
    m_pServerAddress(),
    m_pServerPort(),
    m_plogLevel(),
    m_pModelStoreBackend(),
    m_pModelStoreSqlitePath(),
    m_pPgHost(),
    m_pPgPort(),
    m_pPgDatabase(),
    m_pPgUser(),
    m_pPgPassword(),
    m_pAppDataPath(),
    m_pBacktestPath()
{
    Q_UNUSED(parent);
    setupModelData(rootItem);

    QObject::connect(this, &CSettinsModelData::signalEditLogSettingsCompleted, this, &CSettinsModelData::slotEditSettingLogSettingsComlited, Qt::QueuedConnection);
    QObject::connect(this, &CSettinsModelData::signalEditServerPortCompleted, NHelper::writeServerPort);
    QObject::connect(this, &CSettinsModelData::signalEditServerAddresCompleted, NHelper::writeServerAddress);

    NHelper::initSettings();

    createSettingsView();

    MyLogger::setDebugLevelMask(NHelper::getLoggerMask());

    if (m_treeView) {
        QObject::connect(m_treeView, &QAbstractItemView::doubleClicked,
                         this, [this](const QModelIndex& idx) { onSettingsDoubleClicked(idx); });
    }
}

static QString tooltipForStorageValueIdImpl(quint16 valueColumnId)
{
    switch (valueColumnId) {
    case S_DATA_ID_MODEL_STORE_BACKEND:
        return QStringLiteral(
            "Persistence backend for the strategy tree, catalog, versions, and app metadata (e.g. UI layout). "
            "Choose SQLite (local file) or PostgreSQL (server). Only the relevant connection fields are shown. "
            "Changing this requires restarting the application.");
    case S_DATA_ID_MODEL_STORE_SQLITE_PATH:
        return QStringLiteral(
            "Path to the SQLite database file for ModelStore. "
            "Double-click the value cell to choose a file. After changing, restart the application.");
    case S_DATA_ID_PG_HOST:
        return QStringLiteral(
            "PostgreSQL server host. Used when ModelStore backend is postgresql. "
            "The legacy [DBSettings] block is kept in sync for market-data connections.");
    case S_DATA_ID_PG_PORT:
        return QStringLiteral("TCP port for PostgreSQL (default 5432).");
    case S_DATA_ID_PG_DATABASE:
        return QStringLiteral("PostgreSQL database name for ModelStore.");
    case S_DATA_ID_PG_USER:
        return QStringLiteral("PostgreSQL user name.");
    case S_DATA_ID_PG_PASSWORD:
        return QStringLiteral(
            "PostgreSQL password. Stored in ibtrade.ini; restart may be required after changes.");
    case S_DATA_ID_APP_DATA_PATH:
        return QStringLiteral(
            "SQLite file for application/runtime data (positions, legacy DBHandler tables). "
            "Separate from the model tree. Double-click the value cell to choose a file; restart required.");
    case S_DATA_ID_BACKTEST_PATH:
        return QStringLiteral(
            "SQLite file for backtest runs, history, and results. "
            "Double-click the value cell to choose a file; restart required.");
    default:
        return {};
    }
}

bool CSettinsModelData::storageReconfigurationAllowed() const
{
    if (auto* mm = qobject_cast<CMainModel*>(QObject::parent()))
        return mm->storageReconfigurationAllowed();
    return true;
}

void CSettinsModelData::applyStorageVisibilityForBackend()
{
    if (!m_treeView)
        return;

    QModelIndex storageParent;
    const int rootRows = rowCount(QModelIndex());
    for (int r = 0; r < rootRows; ++r) {
        const QModelIndex idx0 = index(r, 0, QModelIndex());
        TreeItem* it = getItem(idx0);
        if (!it)
            continue;
        if (it->data(0).value.toString() == QStringLiteral("Storage")) {
            storageParent = index(r, 0, QModelIndex());
            break;
        }
    }
    if (!storageParent.isValid())
        return;

    const int n = rowCount(storageParent);
    if (n < 9)
        return;

    StorageBackend b = StorageBackend::Sqlite;
    QString err;
    StorageConfig::parseBackend(m_pModelStoreBackend->value.toString(), &b, &err);
    const bool isPg = (b == StorageBackend::Postgresql);

    // Children: 0 backend, 1 sqlite path, 2–6 PostgreSQL, 7 app data, 8 backtest
    m_treeView->setRowHidden(1, storageParent, isPg);
    for (int r = 2; r <= 6; ++r)
        m_treeView->setRowHidden(r, storageParent, !isPg);
}


QVariant CSettinsModelData::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole && index.column() == 1) {
        TreeItem* item = getItem(index);
        if (item && item->data(1).id == S_DATA_ID_MODEL_STORE_BACKEND) {
            const QString raw = item->data(1).value.toString().trimmed().toLower();
            if (raw == QLatin1String("postgresql") || raw == QLatin1String("postgres"))
                return QStringLiteral("PostgreSQL");
            return QStringLiteral("SQLite");
        }
    }
    if (role == Qt::ToolTipRole) {
        TreeItem* item = getItem(index);
        if (!item)
            return {};
        const quint16 valueId = item->data(1).id;
        if (valueId >= S_DATA_ID_MODEL_STORE_BACKEND && valueId <= S_DATA_ID_BACKTEST_PATH) {
            const QString tip = tooltipForStorageValueIdImpl(valueId);
            if (!tip.isEmpty())
                return tip;
        }
        return {};
    }
    return CTreeViewCustomModel::data(index, role);
}

Qt::ItemFlags CSettinsModelData::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = CTreeViewCustomModel::flags(index);
    TreeItem* item = getItem(index);
    const quint16 valueId = item->data(1).id;
    if (valueId >= S_DATA_ID_MODEL_STORE_BACKEND && valueId <= S_DATA_ID_BACKTEST_PATH) {
        if (!storageReconfigurationAllowed())
            f &= ~Qt::ItemIsEditable;
    }
    return f;
}

void CSettinsModelData::onSettingsDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    const QModelIndex valueIdx = index.column() == 1 ? index : index.sibling(index.row(), 1);
    TreeItem* item = getItem(valueIdx);
    if (!item)
        return;
    const quint16 id = item->data(1).id;
    if (id != S_DATA_ID_MODEL_STORE_SQLITE_PATH && id != S_DATA_ID_APP_DATA_PATH && id != S_DATA_ID_BACKTEST_PATH)
        return;
    if (!storageReconfigurationAllowed()) {
        QMessageBox::warning(m_treeView, QStringLiteral("Storage"),
                               QStringLiteral("Disconnect from the broker and wait for any backtest to finish "
                                              "before changing database files."));
        return;
    }
    const QString current = item->data(1).value.toString();
    QString startPath = current;
    if (!QFileInfo(current).isAbsolute())
        startPath = QDir::current().filePath(current);
    const QFileInfo fi(startPath);
    const QString dir = fi.isFile() || fi.exists() ? fi.absolutePath() : QDir::homePath();
    const QString baseName = fi.isFile() ? fi.fileName() : QStringLiteral("database.sqlite");

    const QString path = QFileDialog::getSaveFileName(
        m_treeView, QStringLiteral("SQLite database file"),
        dir + QLatin1Char('/') + baseName,
        QStringLiteral("SQLite databases (*.sqlite *.db);;All files (*)"));

    if (path.isEmpty() || path == current)
        return;

    if (!setData(valueIdx, path, Qt::EditRole))
        return;
    persistStorageSettingsFromUi();
    QMessageBox::information(m_treeView, QStringLiteral("Storage"),
                             QStringLiteral("Database paths are saved. Restart the application for changes to take effect."));
}

void CSettinsModelData::setupModelData(TreeItem * rootItem)
{

    rootItem->insertColumns(0,2);
    rootItem->addData(0, pItemDataType(new stItemData("Parameter", EVT_TEXT, S_DATA_ID_UNSET)));
    rootItem->addData(1, pItemDataType(new stItemData("Value", EVT_TEXT, S_DATA_ID_UNSET)));

    QList<TreeItem *> parents;
    parents << rootItem;

    TreeItem * parent = parents.last();

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Servers", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QVariant(), EVT_READ_ONLY, S_DATA_ID_UNSET)));

    TreeItem * _parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    m_pServerAddress = pItemDataType(new stItemData("127.0.0.1", EVT_TEXT, S_DATA_ID_SERVER_ADDRESS));
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Address", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    m_pServerPort = pItemDataType(new stItemData(QString::number(CONNECTIONS_SERVER_PORT),
                                                 EVT_TEXT,
                                                 S_DATA_ID_SERVER_PORT));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerAddress);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Port", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_pServerPort);

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Storage", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QVariant(), EVT_READ_ONLY, S_DATA_ID_UNSET)));

    {
        const StorageConfig storageDefaults = StorageConfig::loadDefaults();
        TreeItem * stor = parent->child(parent->childCount() - 1);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pModelStoreBackend = pItemDataType(new stItemData(StorageConfig::backendToString(storageDefaults.modelStore.backend), EVT_TEXT, S_DATA_ID_MODEL_STORE_BACKEND));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("ModelStore backend"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pModelStoreBackend);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pModelStoreSqlitePath = pItemDataType(new stItemData(storageDefaults.modelStore.sqlitePath, EVT_TEXT, S_DATA_ID_MODEL_STORE_SQLITE_PATH));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("ModelStore SQLite file"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pModelStoreSqlitePath);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pPgHost = pItemDataType(new stItemData(storageDefaults.modelStore.postgres.host, EVT_TEXT, S_DATA_ID_PG_HOST));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("PostgreSQL host"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pPgHost);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pPgPort = pItemDataType(new stItemData(storageDefaults.modelStore.postgres.port, EVT_TEXT, S_DATA_ID_PG_PORT));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("PostgreSQL port"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pPgPort);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pPgDatabase = pItemDataType(new stItemData(storageDefaults.modelStore.postgres.database, EVT_TEXT, S_DATA_ID_PG_DATABASE));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("PostgreSQL database"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pPgDatabase);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pPgUser = pItemDataType(new stItemData(storageDefaults.modelStore.postgres.user, EVT_TEXT, S_DATA_ID_PG_USER));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("PostgreSQL user"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pPgUser);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pPgPassword = pItemDataType(new stItemData(storageDefaults.modelStore.postgres.password, EVT_TEXT, S_DATA_ID_PG_PASSWORD));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("PostgreSQL password"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pPgPassword);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pAppDataPath = pItemDataType(new stItemData(storageDefaults.appDataStore.path, EVT_TEXT, S_DATA_ID_APP_DATA_PATH));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("App data SQLite file"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pAppDataPath);
        stor->insertChildren(stor->childCount(), 1, rootItem->columnCount());
        m_pBacktestPath = pItemDataType(new stItemData(storageDefaults.backtestStore.path, EVT_TEXT, S_DATA_ID_BACKTEST_PATH));
        stor->child(stor->childCount() - 1)->addData(0, pItemDataType(new stItemData(QStringLiteral("Backtest SQLite file"), EVT_RO_TEXT, S_DATA_ID_UNSET)));
        stor->child(stor->childCount() - 1)->addData(1, m_pBacktestPath);
    }

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    parent->child(parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Log Setting", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    parent->child(parent->childCount() - 1)->addData(1, pItemDataType(new stItemData(QVariant(), EVT_READ_ONLY, S_DATA_ID_UNSET)));

    m_plogLevel.reserve(S_LOG_LEVEL_COUNT);
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Checked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_ALL)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_FATAL)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_ERROR)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_WARNING)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_INFO)));
    m_plogLevel.append(pItemDataType(new stItemData(Qt::Unchecked, EVT_CECK_BOX, S_DATA_ID_LOG_LEVEL_DEBUG)));


    _parent = parent->child(parent->childCount() - 1);
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("ALL", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_ALL));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Fatal", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_FATAL));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Error", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_ERROR));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Warning", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_WARNING));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Info", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_INFO));
    _parent->insertChildren(_parent->childCount(), 1, rootItem->columnCount());
    _parent->child(_parent->childCount() - 1)->addData(0, pItemDataType(new stItemData("Debug", EVT_RO_TEXT, S_DATA_ID_UNSET)));
    _parent->child(_parent->childCount() - 1)->addData(1, m_plogLevel.at(S_INDEX_LOG_LEVEL_DEBUG));
}

void CSettinsModelData::dataChangeCallback(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> & param)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(param);

    if (topLeft.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(topLeft.internalPointer());
        if(item)
        {
            const auto itemData = item->data(topLeft.column());
            if((S_DATA_ID_LOG_LEVEL_ALL <= itemData.id) && (S_DATA_ID_LOG_LEVEL_DEBUG >= itemData.id))
            {
                emit signalEditLogSettingsCompleted();
            }
            else if(S_DATA_ID_SERVER_ADDRESS == itemData.id)
            {
                signalEditServerAddresCompleted(itemData.value.toString());
            }
            else if(S_DATA_ID_SERVER_PORT == itemData.id)
            {
                signalEditServerPortCompleted(itemData.value.toInt());
            }
            else if (itemData.id >= S_DATA_ID_MODEL_STORE_BACKEND && itemData.id <= S_DATA_ID_BACKTEST_PATH) {
                persistStorageSettingsFromUi();
            }

        }
    }
}

void CSettinsModelData::persistStorageSettingsFromUi()
{
    if (!storageReconfigurationAllowed())
        return;
    StorageConfig c = NHelper::getStorageConfig();
    QString err;
    if (!StorageConfig::parseBackend(m_pModelStoreBackend->value.toString(), &c.modelStore.backend, &err))
        c.modelStore.backend = StorageBackend::Sqlite;
    c.modelStore.sqlitePath = m_pModelStoreSqlitePath->value.toString();
    c.modelStore.postgres.host = m_pPgHost->value.toString();
    c.modelStore.postgres.port = m_pPgPort->value.toInt();
    c.modelStore.postgres.database = m_pPgDatabase->value.toString();
    c.modelStore.postgres.user = m_pPgUser->value.toString();
    c.modelStore.postgres.password = m_pPgPassword->value.toString();
    c.appDataStore.path = m_pAppDataPath->value.toString();
    c.backtestStore.path = m_pBacktestPath->value.toString();
    NHelper::saveStorageConfig(c);
    applyStorageVisibilityForBackend();
}

void CSettinsModelData::slotEditSettingLogSettingsComlited()
{
    quint8 mask = CSettinsModelData::getMaskFromLoggerSettings(this->getLoggerSettings());//CSettinsModelData::getMaskFromLoggerSettings(m_SettingsModel.getLoggerSettings());
    NHelper::writeLoggerMask(mask);
    MyLogger::setDebugLevelMask(mask);
}

void CSettinsModelData::setLoggerSettings(const QVector<bool> _levels)
{
    if(S_LOG_LEVEL_COUNT == _levels.length())
    {
        for (quint8 i = 0; i < _levels.length(); ++i)
        {
            m_plogLevel.at(i)->value = _levels.at(i) ? Qt::Checked : Qt::Unchecked;
        }
    }
}

const QVector<bool> CSettinsModelData::getLoggerSettings()
{
    QVector<bool> _levels;
    _levels.reserve(m_plogLevel.length());
    for (const auto& level : m_plogLevel)
    {
        _levels.append(level->value == Qt::Checked ? true : false);
    }

    return _levels;
}

void CSettinsModelData::setServerSettings(const QString &_addr, const quint16 &_port)
{
    m_pServerAddress->value = _addr;
    m_pServerPort->value = _port;
//    m_pModel->item(INDEX_SEVER_ADDRESS, 1)->setData(_addr, Qt::DisplayRole);
//    m_pModel->item(INDEX_SEVER_PORT, 1)->setData(_port, Qt::DisplayRole);
}


quint8 CSettinsModelData::getMaskFromLoggerSettings(const QVector<bool> &_levels)
{
    quint8 mask = static_cast<quint8>(MyLogger::LL_NONE);

    if(true == _levels.at(S_INDEX_LOG_LEVEL_ALL))
    {
        mask = MyLogger::LL_ALL;
    }
    else {
        if(true == _levels.at(S_INDEX_LOG_LEVEL_INFO))
        {
            mask |= MyLogger::LL_INFO;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_DEBUG))
        {
            mask |= MyLogger::LL_DEBUG;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_ERROR))
        {
            mask |= MyLogger::LL_ERROR;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_FATAL))
        {
            mask |= MyLogger::LL_FATAL;
        }
        if(true == _levels.at(S_INDEX_LOG_LEVEL_WARNING))
        {
            mask |= MyLogger::LL_WARNING;
        }
    }
    return mask;
}

void CSettinsModelData::updateLoggerSettingsArray(quint8 _mask, QVector<bool> & _levels)
{
    if (_mask & MyLogger::LL_DEBUG)
    {
        _levels[S_INDEX_LOG_LEVEL_DEBUG] = true;
    }
    if (_mask & MyLogger::LL_INFO)
    {
        _levels[S_INDEX_LOG_LEVEL_INFO] = true;
    }
    if (_mask & MyLogger::LL_WARNING)
    {
        _levels[S_INDEX_LOG_LEVEL_WARNING] = true;
    }
    if (_mask & MyLogger::LL_ERROR)
    {
        _levels[S_INDEX_LOG_LEVEL_ERROR] = true;
    }
    if (_mask & MyLogger::LL_FATAL)
    {
        _levels[S_INDEX_LOG_LEVEL_FATAL] = true;
    }
}

void CSettinsModelData::createSettingsView()
{
    quint8 _mask = NHelper::getLoggerMask();
    QVector<bool> levelArr(S_LOG_LEVEL_COUNT);

    CSettinsModelData::updateLoggerSettingsArray(_mask, levelArr);

    setLoggerSettings(levelArr);
    setServerSettings(NHelper::getServerAddress(), NHelper::getServerPort());

    const StorageConfig sc = NHelper::getStorageConfig();
    m_pModelStoreBackend->value = StorageConfig::backendToString(sc.modelStore.backend);
    m_pModelStoreSqlitePath->value = sc.modelStore.sqlitePath;
    m_pPgHost->value = sc.modelStore.postgres.host;
    m_pPgPort->value = sc.modelStore.postgres.port;
    m_pPgDatabase->value = sc.modelStore.postgres.database;
    m_pPgUser->value = sc.modelStore.postgres.user;
    m_pPgPassword->value = sc.modelStore.postgres.password;
    m_pAppDataPath->value = sc.appDataStore.path;
    m_pBacktestPath->value = sc.backtestStore.path;

    applyStorageVisibilityForBackend();
}
