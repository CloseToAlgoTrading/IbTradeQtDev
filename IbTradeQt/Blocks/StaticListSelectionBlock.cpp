#include "StaticListSelectionBlock.h"

#include "../Pipeline/BlockSubscriptionUtils.h"
#include "../Pipeline/IDataSubscriptionPort.h"
#include "../Pipeline/PipelineRuntimeContext.h"
#include <QJsonArray>
#include <QJsonObject>

namespace Blocks {

StaticListSelectionBlock::StaticListSelectionBlock(QObject* parent)
    : ISelectionBlock(parent)
{}

QString StaticListSelectionBlock::id() const { return QStringLiteral("static-list-selection"); }
QString StaticListSelectionBlock::name() const { return QStringLiteral("Static List Selection"); }

QString StaticListSelectionBlock::description() const
{
    return QStringLiteral(
        "Keeps symbols that appear in both the configured list and the input universe; "
        "if the universe is empty, uses the configured list alone. Updates subscription desired symbols. "
        "Use PassAllSelectionBlock when no filtering is needed.");
}

QJsonObject StaticListSelectionBlock::config() const
{
    QJsonObject cfg;
    QJsonArray arr;
    for (const auto& s : m_symbols) arr.append(s);
    cfg[QStringLiteral("symbols")] = arr;
    return cfg;
}

void StaticListSelectionBlock::setConfig(const QJsonObject& config)
{
    m_symbols.clear();
    for (const auto& s : config.value(QStringLiteral("symbols")).toArray()) {
        const QString sym = s.toString().trimmed().toUpper();
        if (!sym.isEmpty())
            m_symbols.append(sym);
    }
}

void StaticListSelectionBlock::initialize() {}
void StaticListSelectionBlock::shutdown() {}

QVector<QString> StaticListSelectionBlock::select(const QVector<QString>& universe)
{
    QVector<QString> result;
    if (m_symbols.isEmpty()) {
        result = universe;
    } else if (universe.isEmpty()) {
        result = m_symbols;
    } else {
        for (const auto& s : universe) {
            const QString u = s.trimmed().toUpper();
            if (m_symbols.contains(u))
                result.append(s);
        }
        if (result.isEmpty())
            result = m_symbols;
    }

    if (runtimeContext() && runtimeContext()->subscription) {
        const QString oid = Pipeline::subscriptionOwnerId(this, QStringLiteral("selection:"), id());
        if (result.isEmpty())
            runtimeContext()->subscription->clearOwner(oid);
        else
            runtimeContext()->subscription->setDesiredSymbols(oid, result);
    }
    return result;
}

} // namespace Blocks
