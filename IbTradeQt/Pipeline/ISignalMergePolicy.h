#ifndef PIPELINE_ISIGNALMERGEPOLICY_H
#define PIPELINE_ISIGNALMERGEPOLICY_H

#include <QObject>
#include <QVector>
#include "Contracts.h"

namespace Pipeline {

class ISignalMergePolicy : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~ISignalMergePolicy() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;

    virtual Signal merge(const QVector<Signal>& inputSignals) = 0;
};

} // namespace Pipeline

#endif // PIPELINE_ISIGNALMERGEPOLICY_H
