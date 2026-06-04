#include "PortfolioWorkspace.h"
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

PortfolioWorkspace::PortfolioWorkspace(QWidget* parent)
    : WorkspaceBase(parent)
{
    buildOverviewTab();
    buildPropertiesTab();
    buildInfoTab();
    buildLogsTab();
}

void PortfolioWorkspace::buildOverviewTab()
{
    m_overviewWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_overviewWidget);
    layout->setContentsMargins(Layout::TabContentMargin, Layout::TabContentMargin,
                               Layout::TabContentMargin, Layout::TabContentMargin);
    layout->setSpacing(Layout::SectionSpacing);

    m_ovStrategies = new QLabel("Strategies: --", m_overviewWidget);
    m_ovExposure   = new QLabel("Exposure: --", m_overviewWidget);
    m_ovPnL        = new QLabel("PnL: --", m_overviewWidget);

    layout->addWidget(new QLabel("<b>Strategies</b>", m_overviewWidget));
    layout->addWidget(m_ovStrategies);
    layout->addWidget(new QLabel("<b>Exposure</b>", m_overviewWidget));
    layout->addWidget(m_ovExposure);
    layout->addWidget(new QLabel("<b>Daily PnL</b>", m_overviewWidget));
    layout->addWidget(m_ovPnL);
    layout->addStretch();

    addTab("Overview", m_overviewWidget);
}

void PortfolioWorkspace::buildPropertiesTab()
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

void PortfolioWorkspace::buildInfoTab()
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

void PortfolioWorkspace::buildLogsTab()
{
    auto* logsWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(logsWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* logsText = new QTextEdit(logsWidget);
    logsText->setReadOnly(true);
    layout->addWidget(logsText);
    addTab("Logs", logsWidget);
}

void PortfolioWorkspace::onContextSet()
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

    QString breadcrumb;
    if (m_parentModel)
        breadcrumb = m_parentModel->getName();

    DisplayState ds = DisplayState::Idle;
    if (baseModel) ds = baseModel->resolveDisplayState();
    setHeaderInfo(m_boundModel->getName(), breadcrumb, ds);
    refreshProperties();
    m_tabWidget->setCurrentIndex(0);
}

void PortfolioWorkspace::onContextCleared()
{
    m_header->clear();
    m_metricsStrip->clear();
    m_ovStrategies->setText("Strategies: --");
    m_ovExposure->setText("Exposure: --");
    m_ovPnL->setText("PnL: --");
    while (m_propertiesForm->rowCount() > 0) m_propertiesForm->removeRow(0);
    while (m_infoForm->rowCount() > 0) m_infoForm->removeRow(0);
}

void PortfolioWorkspace::refreshMetrics()
{
    if (!m_boundModel) return;

    QVariantMap info = m_boundModel->genericInfo();
    QList<MetricCard> cards;

    auto dval = [&](const char* key) { return info.value(key).toDouble(); };

    double dailyPnl = dval(MandatoryInfo::Portfolio::DailyPnL);
    QColor pnlColor = dailyPnl >= 0 ? QColor(76, 175, 80) : QColor(229, 57, 53);

    cards.append({"PnL", QString::number(dailyPnl, 'f', 2), pnlColor, ""});
    cards.append({"Exposure (Gross)", QString::number(dval(MandatoryInfo::Portfolio::ExposureGross), 'f', 2), {}, ""});
    cards.append({"Exposure (Net)", QString::number(dval(MandatoryInfo::Portfolio::ExposureNet), 'f', 2), {}, ""});
    cards.append({"Positions", info.value(MandatoryInfo::Portfolio::PositionsCount, 0).toString(), {}, ""});
    cards.append({"Strategies", QString::number(m_boundModel->getModels().size()), {}, ""});

    setMetrics(cards);
    refreshOverview();
    refreshInfo();
}

void PortfolioWorkspace::refreshOverview()
{
    if (!m_boundModel) return;
    QVariantMap info = m_boundModel->genericInfo();
    m_ovStrategies->setText(QStringLiteral("Strategies: %1 (active)").arg(m_boundModel->getModels().size()));
    m_ovExposure->setText(QStringLiteral("Gross Exposure: %1")
        .arg(info.value(MandatoryInfo::Portfolio::ExposureGross, "--").toString()));
    m_ovPnL->setText(QStringLiteral("Daily PnL: %1")
        .arg(info.value(MandatoryInfo::Portfolio::DailyPnL, "--").toString()));
}

void PortfolioWorkspace::refreshProperties()
{
    if (!m_boundModel) return;
    while (m_propertiesForm->rowCount() > 0) m_propertiesForm->removeRow(0);

    const QVariantMap& params = m_boundModel->getParameters();
    for (auto it = params.cbegin(); it != params.cend(); ++it) {
        auto* edit = new QLineEdit(it.value().toString(), m_propertiesWidget);
        QString key = it.key();
        edit->setToolTip(
            QStringLiteral("Edits the '%1' portfolio parameter. The value is saved back to the selected model when editing finishes.")
                .arg(key));
        connect(edit, &QLineEdit::editingFinished, this, [this, edit, key]() {
            if (!m_boundModel) return;
            QVariantMap params = m_boundModel->getParameters();
            params[key] = edit->text();
            m_boundModel->setParameters(params);
        });
        m_propertiesForm->addRow(it.key(), edit);
    }
}

void PortfolioWorkspace::refreshInfo()
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
