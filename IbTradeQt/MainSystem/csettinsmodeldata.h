#ifndef CSETTINSMODELDATA_H
#define CSETTINSMODELDATA_H

#include "treeitem.h"
#include <QSharedPointer>

class CSettinsModelData
{
public:
    CSettinsModelData();

    void setupModelData(TreeItem *rootItem);

private:
    ptrTextDataType m_pServerAddress;
    ptrTextDataType m_pServerPort;
    QVector<ptrTextDataType> m_plogLevel;
};

#endif // CSETTINSMODELDATA_H
