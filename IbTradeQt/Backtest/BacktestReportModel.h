#ifndef BACKTEST_BACKTESTREPORTMODEL_H
#define BACKTEST_BACKTESTREPORTMODEL_H

#include <QJsonObject>
#include <QString>

namespace Backtest {

/// Presentation-ready report tree (independent of HTML engine).
struct BacktestReportModel {
    int         schemaVersion = 1;
    QJsonObject sections; // arbitrary JSON sections for templates / UI

    QJsonObject toJson(const QString& generatorVersion) const;
};

} // namespace Backtest

#endif
