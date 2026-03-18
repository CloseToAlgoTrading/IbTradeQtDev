#include "StrategyManagementPanel.h"
#include "StrategyCatalogPanel.h"
#include "StrategyDetailPanel.h"

#include <QSplitter>
#include <QVBoxLayout>

namespace StrategyMgmt {

StrategyManagementPanel::StrategyManagementPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void StrategyManagementPanel::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(Qt::Horizontal);

    m_catalogPanel = new StrategyCatalogPanel;
    m_detailPanel  = new StrategyDetailPanel;

    splitter->addWidget(m_catalogPanel);
    splitter->addWidget(m_detailPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 520});

    layout->addWidget(splitter);

    // Wire signals from child panels
    connect(m_catalogPanel, &StrategyCatalogPanel::strategySelected,
            this, &StrategyManagementPanel::strategySelected);
    connect(m_catalogPanel, &StrategyCatalogPanel::newStrategyRequested,
            this, &StrategyManagementPanel::newStrategyRequested);
    connect(m_detailPanel, &StrategyDetailPanel::metadataChanged,
            this, &StrategyManagementPanel::metadataChanged);
    connect(m_detailPanel, &StrategyDetailPanel::newVersionRequested,
            this, &StrategyManagementPanel::newVersionRequested);
    connect(m_detailPanel, &StrategyDetailPanel::publishRequested,
            this, &StrategyManagementPanel::publishRequested);
    connect(m_detailPanel, &StrategyDetailPanel::archiveRequested,
            this, &StrategyManagementPanel::archiveRequested);
    connect(m_detailPanel, &StrategyDetailPanel::useInLiveRequested,
            this, &StrategyManagementPanel::useInLiveRequested);
    connect(m_detailPanel, &StrategyDetailPanel::openInBacktestRequested,
            this, &StrategyManagementPanel::openInBacktestRequested);
    connect(m_detailPanel, &StrategyDetailPanel::addBlockRequested,
            this, &StrategyManagementPanel::addBlockRequested);
    connect(m_detailPanel, &StrategyDetailPanel::removeBlockRequested,
            this, &StrategyManagementPanel::removeBlockRequested);
}

void StrategyManagementPanel::populateCatalog(const QJsonArray& catalogEntries,
                                                const QMap<QString, int>& versionCounts)
{
    m_catalogPanel->populate(catalogEntries, versionCounts);
}

void StrategyManagementPanel::showStrategyDetail(const QJsonObject& catalogEntry,
                                                   const QJsonArray& versions)
{
    m_detailPanel->showStrategy(catalogEntry, versions);
}

} // namespace StrategyMgmt
