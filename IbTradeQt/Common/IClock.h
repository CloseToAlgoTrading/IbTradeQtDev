#ifndef COMMON_ICLOCK_H
#define COMMON_ICLOCK_H

#include <QDateTime>

// Clock abstraction for deterministic time in backtesting and unit tests.
// Live code uses WallClock; backtest sessions inject SimulatedClock.
// No thread-local globals, no ambient singletons.

class IClock {
public:
    virtual ~IClock() = default;
    virtual QDateTime now() const = 0;
};

// Live path — delegates to Qt wall clock
class WallClock : public IClock {
public:
    QDateTime now() const override { return QDateTime::currentDateTime(); }
};

// Backtest / test path — time is advanced externally
class SimulatedClock : public IClock {
public:
    void setCurrentTime(const QDateTime& t) { m_current = t; }
    QDateTime now() const override { return m_current; }

private:
    QDateTime m_current;
};

#endif // COMMON_ICLOCK_H
