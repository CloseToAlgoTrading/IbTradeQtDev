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

    // Block selection in catalog tree drives detail panel inspector
    connect(m_catalogPanel, &StrategyCatalogPanel::blockSelected,
            this, [this](const QString& /*strategyId*/,
                         const QString& category, const QString& jsonKey,
                         bool isArray, int arrayIndex) {
        m_detailPanel->showBlockDetails(category, jsonKey, isArray, arrayIndex);
    });

    // Forward add/remove block signals from catalog tree context menu
    connect(m_catalogPanel, &StrategyCatalogPanel::addBlockRequested,
            this, &StrategyManagementPanel::addBlockRequested);
    connect(m_catalogPanel, &StrategyCatalogPanel::removeBlockRequested,
            this, &StrategyManagementPanel::removeBlockRequested);
}

void StrategyManagementPanel::populateCatalog(const QJsonArray& catalogEntries,
                                                const QMap<QString, int>& versionCounts,
                                                const QMap<QString, QJsonObject>& latestConfigs)
{
    m_catalogPanel->populate(catalogEntries, versionCounts, latestConfigs);
}

void StrategyManagementPanel::showStrategyDetail(const QJsonObject& catalogEntry,
                                                   const QJsonArray& versions)
{
    m_detailPanel->showStrategy(catalogEntry, versions);
}

} // namespace StrategyMgmt
