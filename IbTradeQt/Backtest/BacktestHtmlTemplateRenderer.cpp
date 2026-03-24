#include "Backtest/BacktestHtmlTemplateRenderer.h"

namespace Backtest {

QString renderHtmlTemplate(const QString& templateText, const QJsonObject& flatValues)
{
    QString out = templateText;
    for (auto it = flatValues.begin(); it != flatValues.end(); ++it) {
        const QString key   = QStringLiteral("{{") + it.key() + QStringLiteral("}}");
        const QString value = it.value().toVariant().toString();
        out.replace(key, value);
    }
    return out;
}

} // namespace Backtest
