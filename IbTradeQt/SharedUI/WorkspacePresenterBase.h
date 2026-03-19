#ifndef WORKSPACEPRESENTERBASE_H
#define WORKSPACEPRESENTERBASE_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QMetaObject>
#include "ViewModels.h"
#include "MetricsStrip.h"

class CGenericModelApi;
class CBaseModel;

class WorkspacePresenterBase : public QObject
{
    Q_OBJECT
public:
    explicit WorkspacePresenterBase(QObject* parent = nullptr);
    ~WorkspacePresenterBase() override;

    void bind(CGenericModelApi* model, CGenericModelApi* parent);
    void unbind();
    void refresh();

    CGenericModelApi* boundModel()  const { return m_model; }
    CGenericModelApi* parentModel() const { return m_parent; }
    bool isBound() const { return m_model != nullptr; }

signals:
    void headerChanged(const VM::WorkspaceHeader& header);
    void metricsChanged(const QList<MetricCard>& cards);
    void contextBound();
    void contextUnbound();

protected:
    virtual void onBound() = 0;
    virtual void onUnbound() = 0;
    virtual void onRefresh() = 0;

    VM::WorkspaceHeader buildHeader(const QString& title,
                                    const QString& breadcrumb,
                                    CBaseModel* baseModel) const;

    CGenericModelApi* m_model  = nullptr;
    CGenericModelApi* m_parent = nullptr;

private:
    QTimer m_refreshTimer;
    QList<QMetaObject::Connection> m_connections;
};

#endif // WORKSPACEPRESENTERBASE_H
