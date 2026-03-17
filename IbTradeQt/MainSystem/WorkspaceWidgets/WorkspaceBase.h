#ifndef WORKSPACEBASE_H
#define WORKSPACEBASE_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QMetaObject>
#include "cmodelstate.h"

class QVBoxLayout;
class QTabWidget;
class CGenericModelApi;
class CBaseModel;
class WorkspaceHeader;
class MetricsStrip;
struct MetricCard;

class WorkspaceBase : public QWidget
{
    Q_OBJECT
public:
    explicit WorkspaceBase(QWidget* parent = nullptr);
    ~WorkspaceBase() override;

    void setContext(CGenericModelApi* model, CGenericModelApi* parent);
    void clearContext();
    void refreshNow();

    CGenericModelApi* boundModel() const { return m_boundModel; }
    CGenericModelApi* parentModel() const { return m_parentModel; }

protected:
    virtual void onContextSet() = 0;
    virtual void onContextCleared() = 0;
    virtual void refreshMetrics() = 0;

    void addTab(const QString& label, QWidget* content);
    void setHeaderInfo(const QString& title, const QString& breadcrumb, DisplayState state);
    void setMetrics(const QList<MetricCard>& cards);

    WorkspaceHeader* m_header;
    MetricsStrip*    m_metricsStrip;
    QTabWidget*      m_tabWidget;
    QTimer           m_refreshTimer;

    CGenericModelApi* m_boundModel  = nullptr;
    CGenericModelApi* m_parentModel = nullptr;
    QList<QMetaObject::Connection> m_connections;

private:
    QVBoxLayout* m_layout;
};

#endif // WORKSPACEBASE_H
