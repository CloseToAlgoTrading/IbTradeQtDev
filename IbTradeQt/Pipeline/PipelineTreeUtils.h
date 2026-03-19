#pragma once

#include <QJsonObject>

class QTreeWidgetItem;

// Shared utility for populating pipeline category + block nodes in any
// QTreeWidget-based tree.  Used by BacktestStrategySelector and
// StrategyCatalogPanel.  SystemTreeModel uses its own TreeNode but
// follows the same category/block hierarchy.
namespace PipelineTreeUtils {

// Qt::UserRole offsets stored on each block leaf item.
// These match the parameters expected by BlockInspectorPanel::showBlock().
enum ItemRole {
    RoleCategory   = Qt::UserRole + 100,   // QString  display category name
    RoleJsonKey    = Qt::UserRole + 101,    // QString  JSON config key
    RoleIsArray    = Qt::UserRole + 102,    // bool
    RoleArrayIndex = Qt::UserRole + 103,    // int
    RoleIsBlock    = Qt::UserRole + 104,    // bool  true for block leaf nodes
};

// Adds Selection/Alpha/Risk/Rebalance/Execution category nodes as children
// of parentItem, with block leaves under each category.
// Category nodes are bold and non-selectable.
// Block leaves carry UserRole data for category, jsonKey, isArray, arrayIndex.
void populateBlockNodes(QTreeWidgetItem* parentItem,
                        const QJsonObject& pipelineConfig);

} // namespace PipelineTreeUtils
