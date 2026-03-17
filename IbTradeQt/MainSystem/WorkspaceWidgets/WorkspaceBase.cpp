#include "WorkspaceBase.h"
#include "WorkspaceHeader.h"
#include "MetricsStrip.h"
#include "LayoutConstants.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include <QVBoxLayout>
#include <QTabWidget>

WorkspaceBase::WorkspaceBase(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_header      = new WorkspaceHeader(this);
    m_metricsStrip = new MetricsStrip(this);
    m_tabWidget   = new QTabWidget(this);

    m_layout->addWidget(m_header);
    m_layout->addWidget(m_metricsStrip);
    m_layout->addWidget(m_tabWidget, 1);

    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_boundModel) refreshMetrics();
    });
}

WorkspaceBase::~WorkspaceBase()
{
    clearContext();
}

void WorkspaceBase::setContext(CGenericModelApi* model, CGenericModelApi* parent)
{
    if (m_boundModel && m_boundModel != model)
        clearContext();

    m_boundModel  = model;
    m_parentModel = parent;

    // Guard against dangling pointer (D5)
    auto* baseModel = dynamic_cast<CBaseModel*>(model);
    if (baseModel) {
        m_connections.append(
            connect(baseModel, &QObject::destroyed, this, [this]() {
                clearContext();
            }));
    }

    onContextSet();
    refreshNow();
    m_refreshTimer.start();
}

void WorkspaceBase::clearContext()
{
    m_refreshTimer.stop();

    for (auto& conn : m_connections)
        QObject::disconnect(conn);
    m_connections.clear();

    if (m_boundModel)
        onContextCleared();

    m_boundModel  = nullptr;
    m_parentModel = nullptr;
}

void WorkspaceBase::refreshNow()
{
    if (m_boundModel)
        refreshMetrics();
}

void WorkspaceBase::addTab(const QString& label, QWidget* content)
{
    m_tabWidget->addTab(content, label);
}

void WorkspaceBase::setHeaderInfo(const QString& title, const QString& breadcrumb, DisplayState state)
{
    m_header->setTitle(title);
    m_header->setBreadcrumb(breadcrumb);
    m_header->setState(state);
}

void WorkspaceBase::setMetrics(const QList<MetricCard>& cards)
{
    m_metricsStrip->setMetrics(cards);
}
