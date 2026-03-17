#include "WorkspaceHeader.h"
#include "LayoutConstants.h"
#include "ModelStateUtils.h"
#include <QHBoxLayout>
#include <QLabel>

WorkspaceHeader::WorkspaceHeader(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(Layout::WorkspaceHeaderHeight);
    setObjectName("WorkspaceHeader");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(Layout::SectionSpacing, 4, Layout::SectionSpacing, 4);
    layout->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("wsTitle");
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_breadcrumbLabel = new QLabel(this);
    m_breadcrumbLabel->setObjectName("wsBreadcrumb");
    m_breadcrumbLabel->setStyleSheet("color: #8888a0; font-size: 11px;");

    m_stateBadge = new QLabel(this);
    m_stateBadge->setObjectName("wsStateBadge");
    m_stateBadge->setFixedHeight(22);
    m_stateBadge->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_breadcrumbLabel);
    layout->addStretch();
    layout->addWidget(m_stateBadge);
}

void WorkspaceHeader::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
}

void WorkspaceHeader::setBreadcrumb(const QString& breadcrumb)
{
    m_breadcrumbLabel->setText(breadcrumb);
}

void WorkspaceHeader::setState(DisplayState state)
{
    auto info = ModelStateUtils::stateDisplay(state);
    m_stateBadge->setText(QStringLiteral("  %1 %2  ").arg(info.indicator, info.label));
    m_stateBadge->setStyleSheet(
        QStringLiteral("background-color: %1; color: white; border-radius: 4px; padding: 2px 8px; font-size: 11px;")
            .arg(info.color.darker(130).name()));
}

void WorkspaceHeader::clear()
{
    m_titleLabel->clear();
    m_breadcrumbLabel->clear();
    m_stateBadge->clear();
}
