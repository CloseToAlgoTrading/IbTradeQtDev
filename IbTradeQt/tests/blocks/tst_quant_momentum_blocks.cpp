#include "tst_quant_momentum_blocks.h"

#include "Adapters/MockPositionRepository.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/SimpleRebalanceBlock.h"
#include "Pipeline/IHistoricalRead.h"
#include "Pipeline/IMarketDataAccessor.h"
#include "Pipeline/PipelineRuntimeContext.h"
#include "Pipeline/SemanticModelDataMapper.h"
#include "Strategies/Generic/UnifiedModelData.h"

#include <QDateTime>
#include <QJsonObject>
#include <QSignalSpy>
#include <optional>

namespace {

class HistoricalStub : public Pipeline::IHistoricalRead {
public:
    QMap<QString, QVector<Pipeline::HistoricalBarSnapshot>> barsBySymbol;

    QVector<Pipeline::HistoricalBarSnapshot> getBars(
        const QString& symbol,
        const QString& /*resolution*/,
        const QString& /*dataSourceId*/,
        const QDateTime& /*from*/,
        const QDateTime& /*to*/) override
    {
        return barsBySymbol.value(symbol.toUpper());
    }
};

static Pipeline::HistoricalBarSnapshot snap(double close)
{
    Pipeline::HistoricalBarSnapshot s;
    s.timestamp = QDateTime::currentDateTimeUtc();
    s.close = close;
    return s;
}

class MarketDataStub : public Pipeline::IMarketDataAccessor {
public:
    QMap<QString, Pipeline::MarketTick> ticks;

    std::optional<Pipeline::MarketTick> lastTick(const QString& symbol) const override
    {
        auto it = ticks.find(symbol.toUpper());
        if (it == ticks.end())
            return std::nullopt;
        return it.value();
    }
};

} // namespace

void TestQuantMomentumBlocks::momentumSemantic_usesPeriodBarReturnAndTopN()
{
    HistoricalStub hist;
    hist.barsBySymbol[QStringLiteral("UP")] = {snap(100.0), snap(100.0), snap(110.0)};
    hist.barsBySymbol[QStringLiteral("DN")] = {snap(100.0), snap(100.0), snap(102.0)};

    Blocks::MomentumAlphaBlock alpha;
    QJsonObject cfg;
    cfg[QStringLiteral("period")] = 2;
    cfg[QStringLiteral("topN")] = 1;
    cfg[QStringLiteral("threshold")] = 0.001;
    alpha.setConfig(cfg);

    Pipeline::PipelineRuntimeContext ctx;
    ctx.historical = &hist;
    alpha.setRuntimeContext(&ctx);

    Pipeline::ModelDataList in = Pipeline::SemanticMapping::buildModelDataFromSymbols(
        {QStringLiteral("UP"), QStringLiteral("DN")});
    Pipeline::ModelDataList out = alpha.processSemantic(in, QStringLiteral("c1"));

    QVERIFY(out);
    QCOMPARE(out->size(), 1);
    QCOMPARE(out->at(0).symbol, QStringLiteral("UP"));
}

void TestQuantMomentumBlocks::simpleRebalance_equalWeight_usesAllocatedCapital()
{
    MarketDataStub mkt;
    Pipeline::MarketTick t;
    t.bid = 100.0;
    t.ask = 100.0;
    mkt.ticks[QStringLiteral("X")] = t;
    mkt.ticks[QStringLiteral("Y")] = t;

    Pipeline::PipelineRuntimeContext ctx;
    ctx.marketData = &mkt;
    ctx.strategyAllocatedCapital = 10'000.0;

    Blocks::SimpleRebalanceBlock reb;
    QJsonObject cfg;
    cfg[QStringLiteral("equalWeight")] = true;
    reb.setConfig(cfg);
    reb.setRuntimeContext(&ctx);

    Pipeline::ModelDataList in = createDataList();
    in->append(UnifiedModelData(QStringLiteral("X"), DIRECTION_UP, 1.0, 0.0, 0.0));
    in->append(UnifiedModelData(QStringLiteral("Y"), DIRECTION_UP, 1.0, 0.0, 0.0));

    Pipeline::ModelDataList out =
        reb.processSemantic(in, QMap<QString, double>{}, QStringLiteral("c1"));
    QVERIFY(out);
    QCOMPARE(out->size(), 2);
    for (const auto& row : *out) {
        QCOMPARE(row.amount, 50.0);
    }
}

void TestQuantMomentumBlocks::simpleRebalance_rotation_emitsExitForDroppedHoldings()
{
    MarketDataStub mkt;
    Pipeline::MarketTick t;
    t.bid = 100.0;
    t.ask = 100.0;
    mkt.ticks[QStringLiteral("X")] = t;
    mkt.ticks[QStringLiteral("Y")] = t;

    Pipeline::PipelineRuntimeContext ctx;
    ctx.marketData = &mkt;
    ctx.strategyAllocatedCapital = 10'000.0;

    Blocks::SimpleRebalanceBlock reb;
    QJsonObject cfg;
    cfg[QStringLiteral("equalWeight")] = true;
    reb.setConfig(cfg);
    reb.setRuntimeContext(&ctx);

    Pipeline::ModelDataList in = createDataList();
    in->append(UnifiedModelData(QStringLiteral("X"), DIRECTION_UP, 1.0, 0.0, 0.0));
    in->append(UnifiedModelData(QStringLiteral("Y"), DIRECTION_UP, 1.0, 0.0, 0.0));

    QMap<QString, double> pos;
    pos[QStringLiteral("OLD")] = 100.0;
    ctx.holdings = pos;

    Pipeline::ModelDataList out = reb.processSemantic(in, pos, QStringLiteral("c1"));
    QVERIFY(out);
    QCOMPARE(out->size(), 3);

    bool foundExit = false;
    for (const auto& row : *out) {
        if (row.symbol == QStringLiteral("OLD")) {
            QCOMPARE(row.direction, DIRECTION_DOWN);
            QCOMPARE(row.amount, 100.0);
            foundExit = true;
        }
    }
    QVERIFY(foundExit);
}

void TestQuantMomentumBlocks::maxPositionRisk_stopLoss_emitsSellOnce()
{
    MockPositionRepository repo;
    repo.setPosition(1, QStringLiteral("S"), 10.0, 100.0);

    Pipeline::PipelineRuntimeContext ctx;
    ctx.positions = &repo;
    ctx.strategyId = 1;

    Blocks::MaxPositionRiskBlock risk;
    QJsonObject cfg;
    cfg[QStringLiteral("stopLossPercent")] = 5.0;
    cfg[QStringLiteral("maxPositionSize")] = 1e9;
    cfg[QStringLiteral("maxTotalExposure")] = 1e12;
    risk.setConfig(cfg);
    risk.setRuntimeContext(&ctx);

    QSignalSpy spy(&risk, &Pipeline::IRiskBlock::riskSignalGenerated);

    Pipeline::MarketTick tick;
    tick.symbol = QStringLiteral("S");
    tick.bid = 90.0;
    tick.ask = 90.0;
    tick.timestamp = QDateTime::currentDateTimeUtc();

    risk.onTick(tick);
    QCOMPARE(spy.count(), 1);

    risk.onTick(tick);
    QCOMPARE(spy.count(), 1);
}
