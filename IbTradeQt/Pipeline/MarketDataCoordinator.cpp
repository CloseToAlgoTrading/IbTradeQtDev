#include "MarketDataCoordinator.h"

#include <QSet>

namespace Pipeline {

void MarketDataCoordinator::setDesiredSymbols(const QVector<QString>& symbols)
{
    m_desired = symbols;
}

void MarketDataCoordinator::diffAgainstCurrent(const QVector<QString>& currentlySubscribed,
                                               QStringList* outSubscribe,
                                               QStringList* outCancel) const
{
    if (!outSubscribe || !outCancel)
        return;
    outSubscribe->clear();
    outCancel->clear();

    QSet<QString> want;
    for (const QString& s : m_desired)
        want.insert(s.trimmed().toUpper());

    QSet<QString> have;
    for (const QString& s : currentlySubscribed)
        have.insert(s.trimmed().toUpper());

    for (const QString& w : want)
        if (!have.contains(w))
            outSubscribe->append(w);
    for (const QString& h : have)
        if (!want.contains(h))
            outCancel->append(h);
}

} // namespace Pipeline
