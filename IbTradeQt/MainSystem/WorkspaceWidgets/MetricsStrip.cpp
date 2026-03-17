#include "MetricsStrip.h"
#include "LayoutConstants.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

MetricsStrip::MetricsStrip(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(Layout::MetricsStripHeight);
    setObjectName("MetricsStrip");

    m_cardLayout = new QHBoxLayout(this);
    m_cardLayout->setContentsMargins(Layout::SectionSpacing, 4, Layout::SectionSpacing, 4);
    m_cardLayout->setSpacing(Layout::MetricCardSpacing);
    static_cast<QHBoxLayout*>(m_cardLayout)->addStretch();
}

void MetricsStrip::setMetrics(const QList<MetricCard>& cards)
{
    clear();
    rebuildCards(cards);
}

void MetricsStrip::clear()
{
    QLayoutItem* item;
    while ((item = m_cardLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MetricsStrip::rebuildCards(const QList<MetricCard>& cards)
{
    for (const auto& card : cards) {
        auto* frame = new QFrame(this);
        frame->setObjectName("metricCard");
        frame->setMinimumWidth(Layout::MetricCardMinWidth);
        frame->setFrameShape(QFrame::NoFrame);
        frame->setStyleSheet(
            "QFrame#metricCard { background-color: #2a2a3e; border-radius: 4px; padding: 4px 8px; }");

        auto* cardLayout = new QVBoxLayout(frame);
        cardLayout->setContentsMargins(Layout::MetricCardPadding, 2, Layout::MetricCardPadding, 2);
        cardLayout->setSpacing(2);

        auto* labelWidget = new QLabel(card.label, frame);
        labelWidget->setStyleSheet("color: #8888a0; font-size: 10px;");

        auto* valueWidget = new QLabel(card.value, frame);
        QFont valueFont = valueWidget->font();
        valueFont.setPointSize(12);
        valueFont.setBold(true);
        valueWidget->setFont(valueFont);
        if (card.color.isValid())
            valueWidget->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; font-weight: bold;").arg(card.color.name()));
        else
            valueWidget->setStyleSheet("color: #e0e0e0; font-size: 13px; font-weight: bold;");

        if (!card.tooltip.isEmpty())
            frame->setToolTip(card.tooltip);

        cardLayout->addWidget(labelWidget);
        cardLayout->addWidget(valueWidget);

        m_cardLayout->addWidget(frame);
    }
    static_cast<QHBoxLayout*>(m_cardLayout)->addStretch();
}
