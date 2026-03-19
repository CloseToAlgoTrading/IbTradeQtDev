#ifndef SYSTEMTREEDELEGATE_H
#define SYSTEMTREEDELEGATE_H

// Backward-compatibility wrapper — all painting logic now lives in
// StrategyTreeDelegate (SharedUI/).
#include "StrategyTreeDelegate.h"

class SystemTreeDelegate : public StrategyTreeDelegate
{
    Q_OBJECT
public:
    explicit SystemTreeDelegate(QObject* parent = nullptr)
        : StrategyTreeDelegate(parent) {}
};

#endif // SYSTEMTREEDELEGATE_H
