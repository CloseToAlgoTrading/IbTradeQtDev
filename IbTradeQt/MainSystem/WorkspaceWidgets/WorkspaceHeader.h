#ifndef WORKSPACEHEADER_H
#define WORKSPACEHEADER_H

#include <QWidget>
#include "ViewModels.h"

class QLabel;
enum class DisplayState;

class WorkspaceHeader : public QWidget
{
    Q_OBJECT
public:
    explicit WorkspaceHeader(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setBreadcrumb(const QString& breadcrumb);
    void setState(DisplayState state);
    void setStateFromVM(const QString& label, const QString& indicator, const QColor& color);
    void applyHeader(const VM::WorkspaceHeader& header);
    void clear();

private:
    void applyStateBadge(const QString& label, const QString& indicator, const QColor& color);

    QLabel* m_titleLabel;
    QLabel* m_breadcrumbLabel;
    QLabel* m_stateBadge;
};

#endif // WORKSPACEHEADER_H
