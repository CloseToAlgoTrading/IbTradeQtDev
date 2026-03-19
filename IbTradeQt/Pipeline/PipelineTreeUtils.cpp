#include "PipelineTreeUtils.h"
#include "PipelineConstants.h"
#include "BlockRegistry.h"

#include <QTreeWidgetItem>
#include <QJsonArray>
#include <QJsonValue>
#include <QFont>

namespace PipelineTreeUtils {

struct CategoryDef {
    QString displayName;
    QLatin1StringView jsonKey;
    bool isArray;
};

static const CategoryDef kCategories[] = {
    { QStringLiteral("Selection"),  Pipeline::Key::Selection, true  },
    { QStringLiteral("Alpha"),      Pipeline::Key::Alphas,    true  },
    { QStringLiteral("Risk"),       Pipeline::Key::Risks,     true  },
    { QStringLiteral("Rebalance"),  Pipeline::Key::Rebalance, false },
    { QStringLiteral("Execution"),  Pipeline::Key::Execution, false },
};

void populateBlockNodes(QTreeWidgetItem* parentItem,
                        const QJsonObject& pipelineConfig)
{
    for (const auto& cat : kCategories) {
        QJsonValue val = pipelineConfig.value(cat.jsonKey);

        auto* catItem = new QTreeWidgetItem(parentItem);
        catItem->setText(0, cat.displayName);
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
        catItem->setData(0, RoleIsBlock, false);
        QFont f = catItem->font(0);
        f.setBold(true);
        catItem->setFont(0, f);

        if (cat.isArray) {
            QJsonArray arr = val.toArray();
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject block = arr[i].toObject();
                QString blockId = block.value(Pipeline::Key::BlockId).toString();

                auto* blockItem = new QTreeWidgetItem(catItem);
                auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
                blockItem->setText(0, desc ? desc.value().name : blockId);

                blockItem->setData(0, RoleCategory,   cat.displayName);
                blockItem->setData(0, RoleJsonKey,     QString(cat.jsonKey));
                blockItem->setData(0, RoleIsArray,     true);
                blockItem->setData(0, RoleArrayIndex,  i);
                blockItem->setData(0, RoleIsBlock,     true);
            }
        } else if (val.isObject() && !val.toObject().isEmpty()) {
            QJsonObject block = val.toObject();
            QString blockId = block.value(Pipeline::Key::BlockId).toString();

            auto* blockItem = new QTreeWidgetItem(catItem);
            auto desc = Pipeline::BlockRegistry::instance().descriptor(blockId);
            blockItem->setText(0, desc ? desc.value().name : blockId);

            blockItem->setData(0, RoleCategory,   cat.displayName);
            blockItem->setData(0, RoleJsonKey,     QString(cat.jsonKey));
            blockItem->setData(0, RoleIsArray,     false);
            blockItem->setData(0, RoleArrayIndex,  0);
            blockItem->setData(0, RoleIsBlock,     true);
        }
    }
}

} // namespace PipelineTreeUtils
