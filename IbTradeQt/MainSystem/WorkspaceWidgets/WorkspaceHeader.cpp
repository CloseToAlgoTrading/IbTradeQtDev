#include "WorkspaceHeader.h"
#include "LayoutConstants.h"
#include "ModelStateUtils.h"
#include "ThemePalette.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLatin1String>

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
    applyStateBadge(info.label, info.indicator, info.color);
}

void WorkspaceHeader::setStateFromVM(const QString& label, const QString& indicator, const QColor& color)
{
    applyStateBadge(label, indicator, color);
}

void WorkspaceHeader::applyHeader(const VM::WorkspaceHeader& header)
{
    m_titleLabel->setText(header.title);
    m_breadcrumbLabel->setText(header.breadcrumb);
    applyStateBadge(header.stateLabel, header.stateIndicator, header.stateColor);
}

void WorkspaceHeader::applyStateBadge(const QString& label, const QString& indicator, const QColor& color)
{
    m_stateBadge->setText(QStringLiteral("  %1 %2  ").arg(indicator, label));
    // Chrome (radius, padding) also in operations-console.qss #wsStateBadge; colors are runtime-only.
    m_stateBadge->setStyleSheet(QStringLiteral(
        "QLabel#wsStateBadge { background-color: %1; color: %2; border-radius: 4px; padding: 2px 8px; "
        "font-size: 11px; min-height: 22px; }")
        .arg(color.darker(130).name(), QLatin1String(UiTheme::kTextOnAccent)));
}

void WorkspaceHeader::clear()
{
    m_titleLabel->clear();
    m_breadcrumbLabel->clear();
    m_stateBadge->clear();
}
