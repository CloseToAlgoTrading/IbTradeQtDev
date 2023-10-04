// treeitem.cpp
// Implements a class that serves as a container for individual items in a tree model.

#include "treeitem.h"
#include "PortfolioModelDefines.h"

// Constructor: Initializes a tree item with given data and parent item.
TreeItem::TreeItem(const QList<pItemDataType> &data, TreeItem *parent)
    : itemData(data), parentItem(parent), firstModelChildIndexCache(-1)
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
    // Add check for nullptr
    if (itemData.at(column) == nullptr)
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

//    updateFirstModelChildIndex(); // Update cache after insertion.
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

    updateFirstModelChildIndex(); // Update cache after insertion.

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

    if(isModel(value.data()->id))
    {
        parentItem->updateFirstModelChildIndex(); // Update cache after insertion.
    }

    return true;
}

bool TreeItem::isModel(const quint16 id) const
{
    auto _id = PM_ITEM_ID_MASK & id;
    return (_id == PM_ITEM_ACCOUNT)
        || (_id == PM_ITEM_PORTFOLIO)
        || (_id == PM_ITEM_STRATEGY)
        /*|| (_id == PM_ITEM_ALFA_MODEL)*/;
}

void TreeItem::updateFirstModelChildIndex()
{
    firstModelChildIndexCache = -1; // Reset cache.

    // Check each child.
    for (int i = 0; i < childItems.size(); ++i) {
        if(nullptr != childItems[i])
        {
            if (childItems[i]->isModel(childItems[i]->data(0).id)) { // Placeholder condition. Replace with actual check.
                firstModelChildIndexCache = i; // Update cache.
                break;
            }
        }
        else
        {
            break;
        }
    }
}

qint8 TreeItem::getFirstModelChildIndexCache() const
{
    return firstModelChildIndexCache;
}
