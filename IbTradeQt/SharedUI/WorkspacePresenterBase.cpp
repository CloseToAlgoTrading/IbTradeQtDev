#include "WorkspacePresenterBase.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include "ModelStateUtils.h"

WorkspacePresenterBase::WorkspacePresenterBase(QObject* parent)
    : QObject(parent)
{
    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_model) onRefresh();
    });
}

WorkspacePresenterBase::~WorkspacePresenterBase()
{
    unbind();
}

void WorkspacePresenterBase::bind(CGenericModelApi* model, CGenericModelApi* parent)
{
    if (m_model && m_model != model)
        unbind();

    m_model  = model;
    m_parent = parent;

    auto* baseModel = dynamic_cast<CBaseModel*>(model);
    if (baseModel) {
        m_connections.append(
            connect(baseModel, &QObject::destroyed, this, [this]() {
                unbind();
            }));
    }

    onBound();
    emit contextBound();
    refresh();
    m_refreshTimer.start();
}

void WorkspacePresenterBase::unbind()
{
    m_refreshTimer.stop();

    for (auto& conn : m_connections)
        QObject::disconnect(conn);
    m_connections.clear();

    if (m_model)
        onUnbound();

    m_model  = nullptr;
    m_parent = nullptr;
    emit contextUnbound();
}

void WorkspacePresenterBase::refresh()
{
    if (m_model)
        onRefresh();
}

VM::WorkspaceHeader WorkspacePresenterBase::buildHeader(
    const QString& title,
    const QString& breadcrumb,
    CBaseModel* baseModel) const
{
    VM::WorkspaceHeader h;
    h.title      = title;
    h.breadcrumb = breadcrumb;

    if (baseModel) {
        auto ds   = baseModel->resolveDisplayState();
        auto info = ModelStateUtils::stateDisplay(ds);
        h.stateLabel     = info.label;
        h.stateIndicator = info.indicator;
        h.stateColor     = info.color;
    }
    return h;
}
