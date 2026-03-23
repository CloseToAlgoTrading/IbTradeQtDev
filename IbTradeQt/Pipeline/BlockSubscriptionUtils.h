#ifndef PIPELINE_BLOCKSUBSCRIPTIONUTILS_H
#define PIPELINE_BLOCKSUBSCRIPTIONUTILS_H

#include <QString>
#include <QObject>

namespace Pipeline {

/// Stable owner key for IDataSubscriptionPort: JSON "id" on the block (via QObject::objectName), else category+type+address.
inline QString subscriptionOwnerId(const QObject* block, const QString& categoryPrefix, const QString& blockTypeId)
{
    if (block) {
        const QString n = block->objectName().trimmed();
        if (!n.isEmpty())
            return categoryPrefix + n;
    }
    return QStringLiteral("%1%2:%3")
        .arg(categoryPrefix)
        .arg(blockTypeId)
        .arg(block ? reinterpret_cast<quintptr>(block) : quintptr(0), 0, 16);
}

} // namespace Pipeline

#endif
