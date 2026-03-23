#include "StrategyWorkspacePresenter.h"
#include "MetricsStrip.h"
#include "cgenericmodelApi.h"
#include "cbasemodel.h"
#include "cpipelinestrategyadapter.h"
#include "ModelStateUtils.h"
#include "mandatoryFieldKeys.h"
#include "Pipeline/UniverseResolver.h"
#include "Pipeline/StrategyRuntimePolicy.h"
#include <QJsonArray>

StrategyWorkspacePresenter::StrategyWorkspacePresenter(QObject* parent)
    : WorkspacePresenterBase(parent)
{
}

void StrategyWorkspacePresenter::onBound()
{
    auto* base = dynamic_cast<CBaseModel*>(m_model);
    QString breadcrumb;
    if (m_parent)
        breadcrumb = m_parent->getName();

    emit headerChanged(buildHeader(m_model->getName(), breadcrumb, base));

    if (base) {
        connect(base, &CBaseModel::displayStateChanged, this,
                [this](DisplayState, DisplayState newState) {
            auto info = ModelStateUtils::stateDisplay(newState);
            VM::WorkspaceHeader h;
            h.title      = m_model->getName();
            h.breadcrumb = m_parent ? m_parent->getName() : QString();
            h.stateLabel     = info.label;
            h.stateIndicator = info.indicator;
            h.stateColor     = info.color;
            emit headerChanged(h);
        });
    }

    emit propertiesChanged(buildProperties());
    emit assetsChanged(buildAssetSymbols(), buildUniverseReason());
    emit policyChanged(buildPolicySummary());
}

void StrategyWorkspacePresenter::onUnbound()
{
    // Connections are cleaned up by WorkspacePresenterBase::unbind()
}

void StrategyWorkspacePresenter::onRefresh()
{
    emit metricsChanged(buildMetrics());
    emit overviewChanged(buildOverview());
    emit infoChanged(buildInfo());
}

// --- Data builders ---

QList<MetricCard> StrategyWorkspacePresenter::buildMetrics() const
{
    if (!m_model) return {};

    QVariantMap info = m_model->genericInfo();
    QList<MetricCard> cards;

    auto addCard = [&](const QString& label, const QString& key, bool colored = false) {
        double val = info.value(key).toDouble();
        QColor c;
        if (colored)
            c = (val >= 0) ? QColor(34, 197, 94) : QColor(239, 68, 68);
        cards.append({label, QString::number(val, 'f', 2), c, key});
    };

    addCard("PnL", MandatoryInfo::Strategy::DailyPnL, true);
    addCard("Unrealized", MandatoryInfo::Strategy::UnrealizedPnL, true);
    addCard("Realized", MandatoryInfo::Strategy::RealizedPnL, true);
    cards.append({"Positions",
                  info.value(MandatoryInfo::Strategy::PositionsCount, 0).toString(),
                  {}, MandatoryInfo::Strategy::PositionsCount});
    cards.append({"Open Orders",
                  info.value(MandatoryInfo::Strategy::OpenOrdersCount, 0).toString(),
                  {}, MandatoryInfo::Strategy::OpenOrdersCount});
    cards.append({"Drawdown",
                  info.value(MandatoryInfo::Strategy::DrawdownPct, "0.00").toString() + "%",
                  QColor(239, 68, 68), MandatoryInfo::Strategy::DrawdownPct});
    return cards;
}

QList<VM::OverviewField> StrategyWorkspacePresenter::buildOverview() const
{
    if (!m_model) return {};

    QList<VM::OverviewField> fields;
    auto* base = dynamic_cast<CBaseModel*>(m_model);
    QVariantMap info = m_model->genericInfo();

    if (base) {
        auto ds = base->resolveDisplayState();
        auto dispInfo = ModelStateUtils::stateDisplay(ds);
        fields.append({"State",
                       QStringLiteral("%1 %2").arg(dispInfo.indicator, dispInfo.label),
                       dispInfo.color.name()});
    }

    int posCount = info.value(MandatoryInfo::Strategy::PositionsCount).toInt();
    fields.append({"Positions", QString::number(posCount), {}});

    QString lastSignal = info.value(MandatoryInfo::Strategy::LastSignalTime).toString();
    fields.append({"Last Signal", lastSignal.isEmpty() ? "--" : lastSignal, {}});

    QString status = info.value(MandatoryInfo::Strategy::Status).toString();
    if (status == QLatin1String("Warning") || status == QLatin1String("Error"))
        fields.append({"Status", status, "warning"});

    auto* a = adapter();
    if (a) {
        auto p = Pipeline::StrategyRuntimePolicy::fromJson(
            pipelineConfig().value("runtimePolicy").toObject());
        QString evalText = Pipeline::StrategyRuntimePolicy::evalModeLabel(p.evaluationMode);
        if (p.evaluationMode != Pipeline::StrategyRuntimePolicy::EvaluationMode::EveryBarClose)
            evalText += QStringLiteral(" (%1)").arg(p.evaluationIntervalN);
        fields.append({"Evaluation", evalText, "muted"});

        QString rebalText = Pipeline::StrategyRuntimePolicy::rebalModeLabel(p.rebalanceMode);
        if (p.rebalanceMode != Pipeline::StrategyRuntimePolicy::RebalanceMode::Immediate)
            rebalText += QStringLiteral(" (%1)").arg(p.rebalanceIntervalN);
        fields.append({"Rebalance", rebalText, "muted"});
    }

    return fields;
}

QList<VM::ParameterRow> StrategyWorkspacePresenter::buildProperties() const
{
    if (!m_model) return {};

    QList<VM::ParameterRow> rows;
    const QVariantMap& params = m_model->getParameters();
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        rows.append({it.key(), it.value().toString(), true});
    return rows;
}

QList<VM::InfoRow> StrategyWorkspacePresenter::buildInfo() const
{
    if (!m_model) return {};

    QList<VM::InfoRow> rows;
    QVariantMap info = m_model->genericInfo();
    for (auto it = info.cbegin(); it != info.cend(); ++it)
        rows.append({it.key(), it.value().toString()});
    return rows;
}

QStringList StrategyWorkspacePresenter::buildAssetSymbols() const
{
    auto* a = adapter();
    if (!a) return {};

    const QJsonObject& cfg = pipelineConfig();
    QStringList allSymbols;

    auto extractSymbols = [&](const QJsonObject& entry) {
        QJsonObject blockCfg = entry.value("config").toObject();
        for (const auto& s : blockCfg.value("symbols").toArray()) {
            const QString sym = s.toString().trimmed().toUpper();
            if (!sym.isEmpty() && !allSymbols.contains(sym))
                allSymbols.append(sym);
        }
    };

    QJsonValue selVal = cfg.value("selection");
    if (selVal.isArray()) {
        for (const auto& entry : selVal.toArray())
            extractSymbols(entry.toObject());
    } else if (selVal.isObject()) {
        extractSymbols(selVal.toObject());
    }

    return allSymbols;
}

QString StrategyWorkspacePresenter::buildUniverseReason() const
{
    auto* a = adapter();
    if (!a) return {};
    return Pipeline::UniverseResolver::resolve(pipelineConfig()).reason;
}

VM::PolicySummary StrategyWorkspacePresenter::buildPolicySummary() const
{
    auto* a = adapter();
    if (!a) return {};

    auto p = Pipeline::StrategyRuntimePolicy::fromJson(
        pipelineConfig().value("runtimePolicy").toObject());

    VM::PolicySummary vm;
    vm.evalModeLabel       = Pipeline::StrategyRuntimePolicy::evalModeLabel(p.evaluationMode);
    vm.evalIntervalN       = p.evaluationIntervalN;
    vm.rebalModeLabel      = Pipeline::StrategyRuntimePolicy::rebalModeLabel(p.rebalanceMode);
    vm.rebalIntervalN      = p.rebalanceIntervalN;
    vm.accumulateSignals   = p.accumulateAlphaSignals;
    vm.signalExpiryBars    = p.signalExpiryBars;
    vm.riskAlwaysActive    = p.riskAlwaysActive;
    vm.riskCanCancelPending = p.riskCanCancelPendingOrders;
    vm.execImmediate       = p.executionImmediateAfterApproval;
    vm.summaryText         = p.summary();
    return vm;
}

// --- Mutations ---

void StrategyWorkspacePresenter::updateParameter(const QString& key, const QString& value)
{
    if (!m_model) return;
    QVariantMap params = m_model->getParameters();
    params[key] = value;
    m_model->setParameters(params);
}

void StrategyWorkspacePresenter::updateAssetSymbols(const QStringList& symbols)
{
    auto* a = adapter();
    if (!a) return;

    QJsonArray symArr;
    for (const auto& s : symbols) symArr.append(s);

    QJsonObject cfg = pipelineConfig();
    QJsonValue selVal = cfg.value("selection");

    if (selVal.isArray()) {
        QJsonArray arr = selVal.toArray();
        if (!arr.isEmpty()) {
            QJsonObject entry = arr[0].toObject();
            QJsonObject blockCfg = entry.value("config").toObject();
            blockCfg["symbols"] = symArr;
            entry["config"] = blockCfg;
            arr[0] = entry;
            cfg["selection"] = arr;
        }
    } else if (selVal.isObject()) {
        QJsonObject entry = selVal.toObject();
        QJsonObject blockCfg = entry.value("config").toObject();
        blockCfg["symbols"] = symArr;
        entry["config"] = blockCfg;
        cfg["selection"] = entry;
    }

    a->setPipelineConfig(cfg);
}

// --- Helpers ---

CPipelineStrategyAdapter* StrategyWorkspacePresenter::adapter() const
{
    return dynamic_cast<CPipelineStrategyAdapter*>(m_model);
}

QJsonObject StrategyWorkspacePresenter::pipelineConfig() const
{
    auto* a = adapter();
    return a ? a->pipelineConfig() : QJsonObject();
}
