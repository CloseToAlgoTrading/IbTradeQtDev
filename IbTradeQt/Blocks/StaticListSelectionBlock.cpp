#include "StaticListSelectionBlock.h"

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
    return QStringLiteral("Filters universe to a configured list of symbols");
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
    for (const auto& s : config.value(QStringLiteral("symbols")).toArray())
        m_symbols.append(s.toString());
}

void StaticListSelectionBlock::initialize() {}
void StaticListSelectionBlock::shutdown() {}

QVector<QString> StaticListSelectionBlock::select(const QVector<QString>& universe)
{
    if (m_symbols.isEmpty()) return universe;
    if (universe.isEmpty()) return m_symbols;
    QVector<QString> result;
    for (const auto& s : universe) {
        if (m_symbols.contains(s)) result.append(s);
    }
    return result.isEmpty() ? m_symbols : result;
}

} // namespace Blocks
