#include "Backtest/BacktestReportModel.h"

namespace Backtest {

QJsonObject BacktestReportModel::toJson(const QString& generatorVersion) const
{
    QJsonObject o;
    o[QStringLiteral("schemaVersion")]         = schemaVersion;
    o[QStringLiteral("generatorVersion")]      = generatorVersion;
    o[QStringLiteral("metricDefinitionsVersion")] = 1;
    o[QStringLiteral("sections")]              = sections;
    return o;
}

} // namespace Backtest
