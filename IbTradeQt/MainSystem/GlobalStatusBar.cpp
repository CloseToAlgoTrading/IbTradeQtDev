#include "GlobalStatusBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QString>
#include <QStyle>

GlobalStatusBar::GlobalStatusBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(32);
    setObjectName("GlobalStatusBar");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    m_brokerLabel     = new QLabel("Broker: --", this);
    m_dataLabel       = new QLabel("Data: --", this);
    m_engineLabel     = new QLabel("Engine: --", this);
    m_strategiesLabel = new QLabel("Strategies: --/--", this);
    m_alertLabel      = new QLabel("Alerts: 0", this);
    m_timeLabel       = new QLabel("--:--:--", this);

    for (auto* lbl : {m_brokerLabel, m_dataLabel, m_engineLabel,
                      m_strategiesLabel, m_alertLabel, m_timeLabel}) {
        lbl->setObjectName("statusIndicator");
    }

    layout->addWidget(m_brokerLabel);
    layout->addWidget(createSeparator());
    layout->addWidget(m_dataLabel);
    layout->addWidget(createSeparator());
    layout->addWidget(m_engineLabel);
    layout->addWidget(createSeparator());
    layout->addWidget(m_strategiesLabel);
    layout->addWidget(createSeparator());
    layout->addWidget(m_alertLabel);
    layout->addWidget(createSeparator());
    layout->addWidget(m_timeLabel);
    layout->addStretch();
}

QWidget* GlobalStatusBar::createSeparator()
{
    auto* sep = new QFrame(this);
    sep->setObjectName(QStringLiteral("globalStatusSeparator"));
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedWidth(1);
    sep->setFixedHeight(18);
    return sep;
}

void GlobalStatusBar::setConnectionState(const QString& broker, bool connected)
{
    m_brokerLabel->setText(
        QStringLiteral("Broker: %1").arg(broker.isEmpty()
            ? (connected ? "Connected" : "Disconnected")
            : (connected ? broker : broker + " (Disconnected)")));
    m_brokerLabel->setProperty("statusState", connected ? "ok" : "error");
    m_brokerLabel->style()->unpolish(m_brokerLabel);
    m_brokerLabel->style()->polish(m_brokerLabel);
}

void GlobalStatusBar::setMarketDataState(bool live)
{
    m_dataLabel->setText(live ? "Data: Live" : "Data: --");
    m_dataLabel->setProperty("statusState", live ? "ok" : "neutral");
    m_dataLabel->style()->unpolish(m_dataLabel);
    m_dataLabel->style()->polish(m_dataLabel);
}

void GlobalStatusBar::setEngineState(const QString& state)
{
    m_engineLabel->setText(QStringLiteral("Engine: %1").arg(state));
}

void GlobalStatusBar::setActiveStrategies(int active, int total)
{
    m_strategiesLabel->setText(
        QStringLiteral("Strategies: %1/%2").arg(active).arg(total));
}

void GlobalStatusBar::setAlertCount(int count)
{
    m_alertLabel->setText(QStringLiteral("Alerts: %1").arg(count));
    m_alertLabel->setProperty("statusState", count > 0 ? "warning" : "neutral");
    m_alertLabel->style()->unpolish(m_alertLabel);
    m_alertLabel->style()->polish(m_alertLabel);
}

void GlobalStatusBar::setTime(const QString& time)
{
    m_timeLabel->setText(time);
}
