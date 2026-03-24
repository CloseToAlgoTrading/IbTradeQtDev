#include "Backtest/SemanticMomentumPipeline.h"

#include <QJsonArray>

namespace Backtest {

QJsonObject buildSemanticMomentumPipeline(const QStringList& universe,
                                          double              initialCapital,
                                          double              maxPositionShares,
                                          int                 rebalanceEveryNBars,
                                          int                 momentumPeriod,
                                          double              momentumThreshold,
                                          int                 topN)
{
    QJsonArray selSyms;
    for (const QString& s : universe)
        selSyms.append(s);

    QJsonObject pipeline;
    pipeline[QStringLiteral("semanticPipeline")]         = true;
    pipeline[QStringLiteral("semanticModelRebalance")]  = true;
    pipeline[QStringLiteral("strategyAllocatedCapital")] = initialCapital;

    QJsonObject sel;
    sel[QStringLiteral("blockId")] = QStringLiteral("static-list-selection");
    sel[QStringLiteral("config")]  = QJsonObject{{QStringLiteral("symbols"), selSyms}};
    pipeline[QStringLiteral("selection")] = sel;

    QJsonObject alphaCfg;
    alphaCfg[QStringLiteral("period")]        = momentumPeriod;
    alphaCfg[QStringLiteral("threshold")]     = momentumThreshold;
    alphaCfg[QStringLiteral("topN")]            = topN;
    alphaCfg[QStringLiteral("lookbackYears")] = 1;
    alphaCfg[QStringLiteral("resolution")]    = QStringLiteral("Day1");
    alphaCfg[QStringLiteral("dataSourceId")]  = QStringLiteral("yahoo");

    pipeline[QStringLiteral("alphas")] =
        QJsonArray{QJsonObject{{QStringLiteral("blockId"), QStringLiteral("momentum-alpha")},
                                {QStringLiteral("config"), alphaCfg}}};

    pipeline[QStringLiteral("rebalance")] =
        QJsonObject{{QStringLiteral("blockId"), QStringLiteral("simple-rebalance")},
                    {QStringLiteral("config"), QJsonObject{{QStringLiteral("equalWeight"), true}}}};

    pipeline[QStringLiteral("risks")] = QJsonArray{
        QJsonObject{{QStringLiteral("blockId"), QStringLiteral("max-position-risk")},
                    {QStringLiteral("config"),
                     QJsonObject{{QStringLiteral("maxPositionSize"), maxPositionShares},
                                 {QStringLiteral("maxTotalExposure"), 1.0e12}}}}};

    pipeline[QStringLiteral("execution")] =
        QJsonObject{{QStringLiteral("blockId"), QStringLiteral("market-order-execution")},
                    {QStringLiteral("config"), QJsonObject{}}};
    pipeline[QStringLiteral("mergePolicy")] = QString();

    QJsonObject rp;
    rp[QStringLiteral("evaluationMode")]        = QStringLiteral("EveryBarClose");
    rp[QStringLiteral("evaluationIntervalN")]   = 1;
    rp[QStringLiteral("rebalanceMode")]         = QStringLiteral("EveryNBars");
    rp[QStringLiteral("rebalanceIntervalN")]    = rebalanceEveryNBars;
    rp[QStringLiteral("accumulateAlphaSignals")] = false;
    pipeline[QStringLiteral("runtimePolicy")] = rp;

    return pipeline;
}

} // namespace Backtest
