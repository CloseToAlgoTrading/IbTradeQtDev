#ifndef BACKTEST_BACKTESTREPORTMODELBUILDER_H
#define BACKTEST_BACKTESTREPORTMODELBUILDER_H

#include "Backtest/BacktestReportContext.h"
#include "Backtest/BacktestReportModel.h"
#include "Backtest/BacktestResult.h"
#include "Backtest/BacktestStatistics.h"

namespace Backtest {

class BacktestReportModelBuilder {
public:
    BacktestReportModel build(const BacktestResult& r,
                              const BacktestStatistics& stats,
                              const BacktestReportContext& ctx) const;
};

} // namespace Backtest

#endif
