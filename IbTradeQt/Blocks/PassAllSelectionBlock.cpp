#include "PassAllSelectionBlock.h"

namespace Blocks {

PassAllSelectionBlock::PassAllSelectionBlock(QObject* parent)
    : ISelectionBlock(parent)
{}

QString PassAllSelectionBlock::id() const { return QStringLiteral("pass-all-selection"); }
QString PassAllSelectionBlock::name() const { return QStringLiteral("Pass All Selection"); }

QString PassAllSelectionBlock::description() const
{
    return QStringLiteral(
        "Returns the input universe unchanged. Use when the upstream universe is already final; "
        "does not set market-data subscriptions (another block or the runner may). "
        "For a fixed list or intersection with the universe, use StaticListSelectionBlock.");
}

QJsonObject PassAllSelectionBlock::config() const { return {}; }
void PassAllSelectionBlock::setConfig(const QJsonObject&) {}
void PassAllSelectionBlock::initialize() {}
void PassAllSelectionBlock::shutdown() {}

QVector<QString> PassAllSelectionBlock::select(const QVector<QString>& universe)
{
    return universe;
}

} // namespace Blocks
