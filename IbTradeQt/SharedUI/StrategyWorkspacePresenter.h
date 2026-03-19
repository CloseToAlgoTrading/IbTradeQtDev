#ifndef STRATEGYWORKSPACEPRESENTER_H
#define STRATEGYWORKSPACEPRESENTER_H

#include "WorkspacePresenterBase.h"
#include <QJsonObject>
#include <QVariantMap>

class CPipelineStrategyAdapter;

class StrategyWorkspacePresenter : public WorkspacePresenterBase
{
    Q_OBJECT
public:
    explicit StrategyWorkspacePresenter(QObject* parent = nullptr);

    // --- Data extraction (no widgets) ---
    QList<MetricCard>       buildMetrics() const;
    QList<VM::OverviewField> buildOverview() const;
    QList<VM::ParameterRow>  buildProperties() const;
    QList<VM::InfoRow>       buildInfo() const;
    QStringList              buildAssetSymbols() const;
    QString                  buildUniverseReason() const;
    VM::PolicySummary        buildPolicySummary() const;

    // --- Mutations ---
    void updateParameter(const QString& key, const QString& value);
    void updateAssetSymbols(const QStringList& symbols);

signals:
    void overviewChanged(const QList<VM::OverviewField>& fields);
    void propertiesChanged(const QList<VM::ParameterRow>& rows);
    void infoChanged(const QList<VM::InfoRow>& rows);
    void assetsChanged(const QStringList& symbols, const QString& universeReason);
    void policyChanged(const VM::PolicySummary& summary);

protected:
    void onBound() override;
    void onUnbound() override;
    void onRefresh() override;

private:
    CPipelineStrategyAdapter* adapter() const;
    QJsonObject pipelineConfig() const;
};

#endif // STRATEGYWORKSPACEPRESENTER_H
