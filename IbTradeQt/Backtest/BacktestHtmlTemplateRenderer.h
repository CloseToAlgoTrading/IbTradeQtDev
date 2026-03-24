#ifndef BACKTEST_BACKTESTHTMLTEMPLATERENDERER_H
#define BACKTEST_BACKTESTHTMLTEMPLATERENDERER_H

#include <QJsonObject>
#include <QString>

namespace Backtest {

/// Minimal placeholder renderer: replaces {{key}} with string values from a flat JSON object (one level).
QString renderHtmlTemplate(const QString& templateText, const QJsonObject& flatValues);

} // namespace Backtest

#endif
