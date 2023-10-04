#ifndef UNIFIEDMODELDATA_H
#define UNIFIEDMODELDATA_H

#include <QList>
#include <QSharedPointer>

struct UnifiedModelData {
    QString symbol;
    int direction;
    double probability;
    double amount;
};

using DataListPtr = QSharedPointer<QList<UnifiedModelData>>;

DataListPtr createDataList() {
    DataListPtr list = QSharedPointer<QList<UnifiedModelData>>::create();
    return list;
}
#endif // UNIFIEDMODELDATA_H
