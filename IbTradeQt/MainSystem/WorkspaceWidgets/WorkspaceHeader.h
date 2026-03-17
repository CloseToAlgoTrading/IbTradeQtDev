#ifndef WORKSPACEHEADER_H
#define WORKSPACEHEADER_H

#include <QWidget>
#include "cmodelstate.h"

class QLabel;

class WorkspaceHeader : public QWidget
{
    Q_OBJECT
public:
    explicit WorkspaceHeader(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setBreadcrumb(const QString& breadcrumb);
    void setState(DisplayState state);
    void clear();

private:
    QLabel* m_titleLabel;
    QLabel* m_breadcrumbLabel;
    QLabel* m_stateBadge;
};

#endif // WORKSPACEHEADER_H
