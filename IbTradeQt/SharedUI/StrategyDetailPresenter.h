#ifndef STRATEGYDETAILPRESENTER_H
#define STRATEGYDETAILPRESENTER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "ViewModels.h"

class StrategyDetailPresenter : public QObject
{
    Q_OBJECT
public:
    explicit StrategyDetailPresenter(QObject* parent = nullptr);

    // --- Data loading ---
    void loadStrategy(const QJsonObject& catalogEntry, const QJsonArray& versions);
    void selectVersion(int row);
    void clear();

    // --- Queries ---
    QString currentStrategyId() const { return m_strategyId; }
    QJsonArray currentVersions() const { return m_versions; }
    QJsonObject workingConfig() const { return m_workingConfig; }
    bool isConfigDirty() const { return m_configDirty; }

    // --- View-model builders ---
    struct MetadataVM {
        QString name;
        QString kindLabel;
        QString description;
        QString tags;
        QString lifecycleState;
    };
    MetadataVM buildMetadata() const;
    QList<VM::VersionRow> buildVersionRows() const;
    QString versionConfigJson(int row) const;
    QString versionDiff(int row) const;

    // --- Mutations ---
    void updateWorkingConfig(const QJsonObject& config);
    void markDirty();

    // --- Utilities ---
    static QString strategyKindLabel(int kind);
    QString selectedVersionId(int row) const;

signals:
    void metadataReady(const MetadataVM& metadata);
    void versionsReady(const QList<VM::VersionRow>& rows);
    void workingConfigChanged(const QJsonObject& config, bool dirty);

private:
    QString m_strategyId;
    QJsonObject m_catalogEntry;
    QJsonArray m_versions;
    QJsonObject m_workingConfig;
    bool m_configDirty = false;
};

#endif // STRATEGYDETAILPRESENTER_H
