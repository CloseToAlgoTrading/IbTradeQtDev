#include "UiLayoutStore.h"
#include "IModelTreeRepository.h"
#include "ibtradesystemview.h"
#include "EventLogPanel.h"
#include "BacktestUI/BacktestSummaryStatisticsPanel.h"
#include "BacktestUI/BacktestStrategySelector.h"
#include "StrategyManagementUI/StrategyManagementPanel.h"
#include "StrategyManagementUI/StrategyCatalogPanel.h"
#include "StrategyManagementUI/StrategyDetailPanel.h"
#include "StrategyTreePanel.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTimer>
#include <QLoggingCategory>
#include <QDockWidget>

Q_LOGGING_CATEGORY(lcUiLayout, "ui.layout")

namespace {
const char kUiLayoutMetadataKey[] = "ui_layout_v1";

QHeaderView* headerForPersistedView(QAbstractItemView* v)
{
    if (!v)
        return nullptr;
    if (auto* tree = qobject_cast<QTreeView*>(v))
        return tree->header();
    if (auto* table = qobject_cast<QTableView*>(v))
        return table->horizontalHeader();
    return nullptr;
}
} // namespace

UiLayoutStore::UiLayoutStore(QObject* parent)
    : QObject(parent)
    , m_saveTimer(new QTimer(this))
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(400);
    connect(m_saveTimer, &QTimer::timeout, this, &UiLayoutStore::onSaveTimer);
}

void UiLayoutStore::setRepository(IModelTreeRepository* repo)
{
    m_repo = repo;
}

QString UiLayoutStore::bytesToJson(const QByteArray& b)
{
    return QString::fromLatin1(b.toBase64());
}

QByteArray UiLayoutStore::bytesFromJson(const QString& s)
{
    if (s.isEmpty())
        return {};
    return QByteArray::fromBase64(s.toLatin1());
}

void UiLayoutStore::registerSplitter(QSplitter* s)
{
    if (!s || s->objectName().isEmpty())
        return;
    m_splitters.append(s);
    connect(s, &QSplitter::splitterMoved, this, &UiLayoutStore::scheduleSave, Qt::UniqueConnection);
}

void UiLayoutStore::registerHeaderView(QAbstractItemView* v)
{
    if (!v || v->objectName().isEmpty())
        return;
    m_headerViews.append(v);
    QHeaderView* h = headerForPersistedView(v);
    if (!h)
        return;
    connect(h, &QHeaderView::sectionResized, this, &UiLayoutStore::scheduleSave, Qt::UniqueConnection);
    connect(h, &QHeaderView::sectionMoved, this, &UiLayoutStore::scheduleSave, Qt::UniqueConnection);
}

void UiLayoutStore::attachToView(CIBTradeSystemView* view)
{
    if (!view)
        return;

    m_mainWindow = view;
    m_backtestDockHost = view->backtestDockHost();
    m_mainTabWidget = view->mainTabWidget();

    if (QSplitter* s = view->mainSplitter()) {
        s->setObjectName(QStringLiteral("MainLiveSplitter"));
        registerSplitter(s);
    }
    if (QSplitter* s = view->backtestSplitter()) {
        s->setObjectName(QStringLiteral("BacktestSplitter"));
        registerSplitter(s);
    }
    if (auto* sm = view->strategyManagementPanel()) {
        if (QSplitter* s = sm->horizontalSplitter()) {
            s->setObjectName(QStringLiteral("StrategyMgmtSplitter"));
            registerSplitter(s);
        }
    }

    if (m_mainTabWidget) {
        connect(m_mainTabWidget, &QTabWidget::currentChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
    }

    if (auto* tv = view->getPortfolioConfigTreeView()) {
        tv->setObjectName(QStringLiteral("SystemTreeView"));
        registerHeaderView(tv);
    }

    if (auto* bt = view->backtestStrategySelector()) {
        if (auto* tv = bt->strategyTreeView()) {
            tv->setObjectName(QStringLiteral("BacktestStrategyTree"));
            registerHeaderView(tv);
        }
    }

    if (auto* sm = view->strategyManagementPanel()) {
        if (auto* cat = sm->catalogPanel()) {
            if (auto* tp = cat->treePanel()) {
                if (auto* tv = tp->treeView()) {
                    tv->setObjectName(QStringLiteral("StrategyCatalogTree"));
                    registerHeaderView(tv);
                }
            }
        }
        if (auto* det = sm->detailPanel()) {
            if (auto* tw = det->versionTable()) {
                tw->setObjectName(QStringLiteral("StrategyVersionTable"));
                registerHeaderView(tw); // QTableWidget inherits QAbstractItemView
            }
        }
    }

    if (auto* dock = view->eventLogPanel()) {
        connect(dock, &QDockWidget::dockLocationChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
        connect(dock, &QDockWidget::topLevelChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);

        dock->enumerateLogTables([this](QTableView* t, const QString& name) {
            if (!t)
                return;
            t->setObjectName(name);
            registerHeaderView(t);
        });
    }

    if (auto* dd = view->diagramDock()) {
        connect(dd, &QDockWidget::dockLocationChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
        connect(dd, &QDockWidget::topLevelChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
    }

    if (auto* statsDock = view->backtestSummaryDock()) {
        connect(statsDock, &QDockWidget::dockLocationChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
        connect(statsDock, &QDockWidget::topLevelChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);
        connect(statsDock, &QDockWidget::visibilityChanged, this, &UiLayoutStore::scheduleSave,
                Qt::UniqueConnection);

        if (auto* panel = qobject_cast<BacktestUI::BacktestSummaryStatisticsPanel*>(
                statsDock->widget())) {
            connect(panel, &BacktestUI::BacktestSummaryStatisticsPanel::headerStateRestorable,
                    this, &UiLayoutStore::restorePendingHeaders, Qt::UniqueConnection);
            if (auto* table = panel->tableView()) {
                table->setObjectName(QStringLiteral("BacktestSummaryStatisticsTable"));
                registerHeaderView(table);
            }
        }
    }
}

void UiLayoutStore::load()
{
    m_pendingHeaderStates.clear();
    if (!m_repo || !m_mainWindow)
        return;

    const QString jsonStr = m_repo->metadata(QString::fromLatin1(kUiLayoutMetadataKey));
    if (jsonStr.isEmpty())
        return;

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(lcUiLayout) << "UiLayoutStore::load: invalid JSON:" << err.errorString();
        return;
    }

    QJsonObject root = doc.object();
    if (root.value(QStringLiteral("schema")).toInt(0) != kJsonSchema)
        qCWarning(lcUiLayout) << "UiLayoutStore::load: schema mismatch, attempting best-effort restore";

    QJsonObject mw = root.value(QStringLiteral("mainWindow")).toObject();
    const QByteArray geom = bytesFromJson(mw.value(QStringLiteral("geometry")).toString());
    const QByteArray st = bytesFromJson(mw.value(QStringLiteral("state")).toString());
    if (!geom.isEmpty())
        m_mainWindow->restoreGeometry(geom);
    if (!st.isEmpty())
        m_mainWindow->restoreState(st, kQtStateVersion);

    const QByteArray backtestDockHostState =
        bytesFromJson(root.value(QStringLiteral("backtestDockHostState")).toString());
    if (m_backtestDockHost && !backtestDockHostState.isEmpty())
        m_backtestDockHost->restoreState(backtestDockHostState, kQtStateVersion);

    QJsonObject splitters = root.value(QStringLiteral("splitters")).toObject();
    for (const QString& key : splitters.keys()) {
        const QByteArray state = bytesFromJson(splitters.value(key).toString());
        if (state.isEmpty())
            continue;
        for (const auto& sp : m_splitters) {
            if (sp && sp->objectName() == key) {
                sp->restoreState(state);
                break;
            }
        }
    }

    if (m_mainTabWidget) {
        const int idx = root.value(QStringLiteral("mainTabIndex")).toInt(-1);
        if (idx >= 0 && idx < m_mainTabWidget->count())
            m_mainTabWidget->setCurrentIndex(idx);
    }

    QJsonObject headers = root.value(QStringLiteral("headers")).toObject();
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        m_pendingHeaderStates[it.key()] = bytesFromJson(it.value().toString());
    }

    restorePendingHeaders();
}

void UiLayoutStore::restorePendingHeaders()
{
    for (const auto& v : m_headerViews) {
        if (!v)
            continue;
        const QString name = v->objectName();
        if (name.isEmpty())
            continue;
        auto it = m_pendingHeaderStates.find(name);
        if (it == m_pendingHeaderStates.end() || it.value().isEmpty())
            continue;
        if (QHeaderView* h = headerForPersistedView(v.get())) {
            if (h->count() > 0 && h->restoreState(it.value()))
                m_pendingHeaderStates.erase(it);
        }
    }
}

void UiLayoutStore::save()
{
    if (!m_repo || !m_mainWindow)
        return;

    QJsonObject root;
    root[QStringLiteral("schema")] = kJsonSchema;

    QJsonObject mw;
    mw[QStringLiteral("geometry")] = bytesToJson(m_mainWindow->saveGeometry());
    mw[QStringLiteral("state")] = bytesToJson(m_mainWindow->saveState(kQtStateVersion));
    root[QStringLiteral("mainWindow")] = mw;
    if (m_backtestDockHost)
        root[QStringLiteral("backtestDockHostState")] =
            bytesToJson(m_backtestDockHost->saveState(kQtStateVersion));

    QJsonObject splitters;
    for (const auto& sp : m_splitters) {
        if (!sp || sp->objectName().isEmpty())
            continue;
        splitters[sp->objectName()] = bytesToJson(sp->saveState());
    }
    root[QStringLiteral("splitters")] = splitters;

    if (m_mainTabWidget)
        root[QStringLiteral("mainTabIndex")] = m_mainTabWidget->currentIndex();

    QJsonObject headers;
    for (const auto& v : m_headerViews) {
        if (!v)
            continue;
        const QString name = v->objectName();
        if (name.isEmpty())
            continue;
        if (QHeaderView* h = headerForPersistedView(v.get())) {
            if (h->count() > 0)
                headers[name] = bytesToJson(h->saveState());
        }
    }
    root[QStringLiteral("headers")] = headers;

    QJsonDocument doc(root);
    m_repo->setMetadata(QString::fromLatin1(kUiLayoutMetadataKey),
                        QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void UiLayoutStore::scheduleSave()
{
    if (m_saveTimer)
        m_saveTimer->start();
}

void UiLayoutStore::onSaveTimer()
{
    save();
}

void UiLayoutStore::clearPersistedLayout()
{
    if (m_repo)
        m_repo->setMetadata(QString::fromLatin1(kUiLayoutMetadataKey), QString());
    m_pendingHeaderStates.clear();
}
