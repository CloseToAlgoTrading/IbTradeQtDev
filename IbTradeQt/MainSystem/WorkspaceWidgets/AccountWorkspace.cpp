#include "AccountWorkspace.h"
#include "LayoutConstants.h"
#include "MetricsStrip.h"
#include "WorkspaceHeader.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextEdit>

AccountWorkspace::AccountWorkspace(QWidget* parent)
    : WorkspaceBase(parent)
{
    buildOverviewTab();
    buildPropertiesTab();
    buildInfoTab();
    buildLogsTab();
}

void AccountWorkspace::buildOverviewTab()
{
    m_overviewWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_overviewWidget);
    layout->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                               Layout::TabContentMargin, Layout::TabContentMargin);
    layout->setSpacing(Layout::SectionSpacing);

    m_ovBrokerStatus = new QLabel("Broker: --", m_overviewWidget);
    m_ovPortfolios   = new QLabel("Portfolios: --", m_overviewWidget);
    m_ovNetLiq       = new QLabel("Net Liquidation: --", m_overviewWidget);

    layout->addWidget(new QLabel("<b>Connection</b>", m_overviewWidget));
    layout->addWidget(m_ovBrokerStatus);
    layout->addWidget(new QLabel("<b>Portfolios</b>", m_overviewWidget));
    layout->addWidget(m_ovPortfolios);
    layout->addWidget(new QLabel("<b>Net Liquidation</b>", m_overviewWidget));
    layout->addWidget(m_ovNetLiq);
    layout->addStretch();

    addTab("Overview", m_overviewWidget);
}

void AccountWorkspace::buildPropertiesTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_propertiesWidget = new QWidget(scroll);
    m_propertiesForm = new QFormLayout(m_propertiesWidget);
    m_propertiesForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                          Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_propertiesWidget);
    addTab("Properties", scroll);
}

void AccountWorkspace::buildInfoTab()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_infoWidget = new QWidget(scroll);
    m_infoForm = new QFormLayout(m_infoWidget);
    m_infoForm->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                                    Layout::TabContentMargin, Layout::TabContentMargin);
    scroll->setWidget(m_infoWidget);
    addTab("Info", scroll);
}

void AccountWorkspace::buildLogsTab()
{
    auto* logsWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(logsWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* logsText = new QTextEdit(logsWidget);
    logsText->setReadOnly(true);
    layout->addWidget(logsText);
    addTab("Logs", logsWidget);
}

void AccountWorkspace::onContextSet()
{
    auto* baseModel = dynamic_cast<CBaseModel*>(m_boundModel);
    if (baseModel) {
        m_connections.append(
            connect(baseModel, &CBaseModel::displayStateChanged, this,
                    [this](DisplayState, DisplayState newState) {
                        m_header->setState(newState);
                        refreshOverview();
                    }));
    }

    DisplayState ds = DisplayState::Idle;
    if (baseModel) ds = baseModel->resolveDisplayState();
    setHeaderInfo(m_boundModel->getName(), "Account", ds);
    refreshProperties();
    m_tabWidget->setCurrentIndex(0);
}

void AccountWorkspace::onContextCleared()
{
    m_header->clear();
    m_metricsStrip->clear();
    m_ovBrokerStatus->setText("Broker: --");
    m_ovPortfolios->setText("Portfolios: --");
    m_ovNetLiq->setText("Net Liquidation: --");
    while (m_propertiesForm->rowCount() > 0) m_propertiesForm->removeRow(0);
    while (m_infoForm->rowCount() > 0) m_infoForm->removeRow(0);
}

void AccountWorkspace::refreshMetrics()
{
    if (!m_boundModel) return;

    QVariantMap info = m_boundModel->genericInfo();
    QList<MetricCard> cards;

    auto dval = [&](const char* key) { return info.value(key).toDouble(); };

    cards.append({"Net Liq", QString::number(dval(MandatoryInfo::Account::NetLiquidation), 'f', 2), {}, ""});
    cards.append({"Cash", QString::number(dval(MandatoryInfo::Account::CashBalance), 'f', 2), {}, ""});
    cards.append({"Buying Power", QString::number(dval(MandatoryInfo::Account::BuyingPower), 'f', 2), {}, ""});
    cards.append({"Available", QString::number(dval(MandatoryInfo::Account::AvailableFunds), 'f', 2), {}, ""});
    cards.append({"Portfolios", QString::number(m_boundModel->getModels().size()), {}, ""});

    setMetrics(cards);
    refreshOverview();
    refreshInfo();
}

void AccountWorkspace::refreshOverview()
{
    if (!m_boundModel) return;
    QVariantMap info = m_boundModel->genericInfo();
    m_ovBrokerStatus->setText(QStringLiteral("Broker: %1").arg(
        info.value(MandatoryInfo::Account::BrokerName, "--").toString()));
    m_ovPortfolios->setText(QStringLiteral("Portfolios: %1").arg(m_boundModel->getModels().size()));
    m_ovNetLiq->setText(QStringLiteral("Net Liquidation: %1").arg(
        info.value(MandatoryInfo::Account::NetLiquidation, "--").toString()));
}

void AccountWorkspace::refreshProperties()
{
    if (!m_boundModel) return;
    while (m_propertiesForm->rowCount() > 0) m_propertiesForm->removeRow(0);

    const QVariantMap& params = m_boundModel->getParameters();
    for (auto it = params.cbegin(); it != params.cend(); ++it) {
        auto* edit = new QLineEdit(it.value().toString(), m_propertiesWidget);
        QString key = it.key();
        connect(edit, &QLineEdit::editingFinished, this, [this, edit, key]() {
            if (!m_boundModel) return;
            QVariantMap params = m_boundModel->getParameters();
            params[key] = edit->text();
            m_boundModel->setParameters(params);
        });
        m_propertiesForm->addRow(it.key(), edit);
    }
}

void AccountWorkspace::refreshInfo()
{
    if (!m_boundModel) return;
    while (m_infoForm->rowCount() > 0) m_infoForm->removeRow(0);

    QVariantMap info = m_boundModel->genericInfo();
    for (auto it = info.cbegin(); it != info.cend(); ++it) {
        auto* label = new QLabel(it.value().toString(), m_infoWidget);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_infoForm->addRow(it.key(), label);
    }
}
