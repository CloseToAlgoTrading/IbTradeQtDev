#ifndef UNIFIEDMODELDATA_H
#define UNIFIEDMODELDATA_H

#include <QList>
#include <QSharedPointer>

struct UnifiedModelData {
    QString symbol;
    int direction;
    double probability;
    double amount;

    // Constructor
    UnifiedModelData(const QString& symbol, int direction, double probability, double amount)
        : symbol(symbol), direction(direction), probability(probability), amount(amount) {}
};


using DataListPtr = QSharedPointer<QList<UnifiedModelData>>;

DataListPtr createDataList();

Q_DECLARE_METATYPE(UnifiedModelData);
Q_DECLARE_METATYPE(DataListPtr);

#endif // UNIFIEDMODELDATA_H


