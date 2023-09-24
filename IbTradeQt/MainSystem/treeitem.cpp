// treeitem.cpp
// Implements a class that serves as a container for individual items in a tree model.

#include "treeitem.h"

// Constructor: Initializes a tree item with given data and parent item.
TreeItem::TreeItem(const QList<pItemDataType> &data, TreeItem *parent)
    : itemData(data), parentItem(parent)
{}

// Destructor: Deletes all child items.
TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

// Retrieve a child item at the given index. Returns nullptr if index is invalid.
TreeItem *TreeItem::child(int number)
{
    if (number < 0 || number >= childItems.size())
        return nullptr;
    return childItems.at(number);
}

// Returns the number of child items.
int TreeItem::childCount() const
{
    return childItems.count();
}

// Returns the index of this item in its parent item's list of children.
int TreeItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return 0;
}

// Returns the number of columns of data held by this item.
int TreeItem::columnCount() const
{
    return itemData.count();
}

// Returns the data at the given column index.
stItemData TreeItem::data(int column) const
{
    if (column < 0 || column >= itemData.size())
        return stItemData();
    return *(itemData.at(column));
}

// Inserts children at the given position.
bool TreeItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QList<pItemDataType> data(columns);
        TreeItem *item = new TreeItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

// Inserts columns at the given position.
bool TreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, pItemDataType(new stItemData(QVariant(), 0, 0)));

    for (TreeItem *child : std::as_const(childItems))
        child->insertColumns(position, columns);

    return true;
}

// Returns the parent item.
TreeItem *TreeItem::parent()
{
    return parentItem;
}

// Removes children at the given position.
bool TreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

// Removes columns at the given position.
bool TreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    for (TreeItem *child : std::as_const(childItems))
        child->removeColumns(position, columns);

    return true;
}

// Sets the data at the given column.
bool TreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column]->value = value;
    return true;
}

// Adds or replaces data at the given column.
bool TreeItem::addData(int column, const pItemDataType &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}
