#include "SubscriptionRequestStore.h"

#include <QMutexLocker>
#include <QSet>

namespace Pipeline {

void SubscriptionRequestStore::setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols)
{
    setDesiredSymbolsWithKinds(ownerId, symbols, subscriptionKindMask(SubscriptionKind::TopOfBook));
}

void SubscriptionRequestStore::setDesiredSymbolsWithKinds(const QString& ownerId,
                                                            const QVector<QString>& symbols,
                                                            quint32 kindMask)
{
    QMutexLocker locker(&m_mutex);
    QHash<QString, quint32> perSym;
    for (const QString& s : symbols) {
        if (!s.isEmpty())
            perSym.insert(s.trimmed().toUpper(), kindMask);
    }
    m_byOwner[ownerId] = perSym;
}

void SubscriptionRequestStore::clearOwner(const QString& ownerId)
{
    QMutexLocker locker(&m_mutex);
    m_byOwner.remove(ownerId);
}

void SubscriptionRequestStore::clearAll()
{
    QMutexLocker locker(&m_mutex);
    m_byOwner.clear();
}

QVector<QString> SubscriptionRequestStore::mergeUnion(const QVector<QString>& base) const
{
    QMutexLocker locker(&m_mutex);
    QSet<QString> seen;
    QVector<QString> out;
    auto append = [&](const QString& s) {
        const QString u = s.trimmed().toUpper();
        if (u.isEmpty() || seen.contains(u))
            return;
        seen.insert(u);
        out.append(u);
    };
    for (const QString& s : base)
        append(s);
    for (auto it = m_byOwner.constBegin(); it != m_byOwner.constEnd(); ++it) {
        for (auto it2 = it.value().constBegin(); it2 != it.value().constEnd(); ++it2)
            append(it2.key());
    }
    return out;
}

QHash<QString, quint32> SubscriptionRequestStore::mergeSymbolKindMasks(const QVector<QString>& baseSymbols) const
{
    QMutexLocker locker(&m_mutex);
    QHash<QString, quint32> merged;
    for (const QString& s : baseSymbols) {
        const QString u = s.trimmed().toUpper();
        if (!u.isEmpty())
            merged[u] |= subscriptionKindMask(SubscriptionKind::TopOfBook);
    }
    for (auto it = m_byOwner.constBegin(); it != m_byOwner.constEnd(); ++it) {
        for (auto it2 = it.value().constBegin(); it2 != it.value().constEnd(); ++it2)
            merged[it2.key()] |= it2.value();
    }
    return merged;
}

} // namespace Pipeline
