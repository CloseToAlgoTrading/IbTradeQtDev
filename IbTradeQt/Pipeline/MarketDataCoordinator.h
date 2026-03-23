#ifndef PIPELINE_MARKETDATACOORDINATOR_H
#define PIPELINE_MARKETDATACOORDINATOR_H

#include <QStringList>
#include <QVector>

namespace Pipeline {

/// Owns the union of symbols that should be subscribed for live market data (Option C).
class MarketDataCoordinator {
public:
    void setDesiredSymbols(const QVector<QString>& symbols);

    QVector<QString> desiredSymbols() const { return m_desired; }

    /// Returns symbols that need subscribe (want - have) and cancel (have - want).
    void diffAgainstCurrent(const QVector<QString>& currentlySubscribed,
                            QStringList* outSubscribe,
                            QStringList* outCancel) const;

private:
    QVector<QString> m_desired;
};

} // namespace Pipeline

#endif // PIPELINE_MARKETDATACOORDINATOR_H
