#ifndef UNIFIEDMODELDATA_H
#define UNIFIEDMODELDATA_H

#include <QList>
#include <QSharedPointer>

enum eDirection {
    DIRECTION_DOWN = 0,
    DIRECTION_UP = 1,
    DIRECTION_FLAT = 2,
    DIRECTION_UNDEFINED = 3
};

struct UnifiedModelData {
    QString symbol;
    eDirection direction;
    double probability;
    double amount;

    // Constructor
    UnifiedModelData(const QString& symbol, eDirection direction, double probability, double amount)
        : symbol(symbol), direction(direction), probability(probability), amount(amount) {}
};


using DataListPtr = QSharedPointer<QList<UnifiedModelData>>;

DataListPtr createDataList();

Q_DECLARE_METATYPE(UnifiedModelData);
Q_DECLARE_METATYPE(DataListPtr);

#endif // UNIFIEDMODELDATA_H


