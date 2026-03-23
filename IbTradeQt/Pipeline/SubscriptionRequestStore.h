#ifndef PIPELINE_SUBSCRIPTIONREQUESTSTORE_H
#define PIPELINE_SUBSCRIPTIONREQUESTSTORE_H

#include "IDataSubscriptionPort.h"
#include <QHash>
#include <QMutex>
#include <QString>
#include <QVector>

namespace Pipeline {

/// Thread-safe owner → (symbol → kind mask); merged into computeTradeableSymbolSet by the strategy adapter.
class SubscriptionRequestStore {
public:
    void setDesiredSymbols(const QString& ownerId, const QVector<QString>& symbols);
    void setDesiredSymbolsWithKinds(const QString& ownerId, const QVector<QString>& symbols, quint32 kindMask);
    void clearOwner(const QString& ownerId);
    void clearAll();

    /// Deduped union: base symbols first, then all owner contributions (stable order).
    QVector<QString> mergeUnion(const QVector<QString>& base) const;

    /// Per-symbol OR of kind masks from block requests; base symbols get TopOfBook by default.
    QHash<QString, quint32> mergeSymbolKindMasks(const QVector<QString>& baseSymbols) const;

private:
    mutable QMutex m_mutex;
    /// ownerId → (symbol → kind bitmask)
    QHash<QString, QHash<QString, quint32>> m_byOwner;
};

} // namespace Pipeline

#endif
