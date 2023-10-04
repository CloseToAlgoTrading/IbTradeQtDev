// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QVariant>
#include <QList>
#include "TreeItemDataTypesDef.h"

class TreeItem;
//! [0]
class TreeItem
{
public:
    explicit TreeItem(const QList<pItemDataType> &data, TreeItem * parent = nullptr);
    ~TreeItem();

    TreeItem * child(int number);
    int childCount() const;
    int columnCount() const;
    stItemData data(int column) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    TreeItem * parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);
    bool addData(int column, const pItemDataType &value);

    bool isModel(const quint16 id) const;
    qint8 getFirstModelChildIndexCache() const;
    void updateFirstModelChildIndex();

private:
    QList<TreeItem *> childItems;
    QList<pItemDataType> itemData;
    TreeItem * parentItem;
    qint8 firstModelChildIndexCache;
};
//! [0]

#endif // TREEITEM_H
