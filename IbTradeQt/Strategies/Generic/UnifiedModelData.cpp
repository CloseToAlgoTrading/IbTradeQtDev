#include "UnifiedModelData.h"

DataListPtr createDataList()
{
    DataListPtr list = QSharedPointer<QList<UnifiedModelData>>::create();
    return list;
}
