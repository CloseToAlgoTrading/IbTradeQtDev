#ifndef PIPELINE_BLOCKREGISTRY_H
#define PIPELINE_BLOCKREGISTRY_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <functional>
#include "Scope.h"
#include "../Common/Expected.h"

namespace Pipeline {

struct BlockDescriptor {
    QString id;
    QString name;
    QString category;  // "Selection", "Alpha", "Rebalance", "Risk", "Execution", "MergePolicy"
    QString description;
    Scope scope = Scope::Strategy;
    QJsonObject defaultConfig;
    std::function<QObject*()> factory;
};

class BlockRegistry : public QObject {
    Q_OBJECT

public:
    static BlockRegistry& instance() {
        static BlockRegistry registry;
        return registry;
    }

    void registerBlock(const BlockDescriptor& descriptor) {
        m_blocks[descriptor.id] = descriptor;
        emit blockRegistered(descriptor.id, descriptor.category);
    }

    bool unregisterBlock(const QString& blockId) {
        return m_blocks.remove(blockId) > 0;
    }

    bool contains(const QString& blockId) const {
        return m_blocks.contains(blockId);
    }

    Expected<BlockDescriptor, Error> descriptor(const QString& blockId) const {
        auto it = m_blocks.find(blockId);
        if (it == m_blocks.end()) {
            return make_unexpected(Error{
                ErrorCode::NotFound,
                "Block '" + blockId.toStdString() + "' not found in registry",
                "BlockRegistry::descriptor"
            });
        }
        return it.value();
    }

    Expected<QObject*, Error> createBlock(const QString& blockId) const {
        auto it = m_blocks.find(blockId);
        if (it == m_blocks.end()) {
            return make_unexpected(Error{
                ErrorCode::NotFound,
                "Block '" + blockId.toStdString() + "' not found in registry",
                "BlockRegistry::createBlock"
            });
        }
        if (!it.value().factory) {
            return make_unexpected(Error{
                ErrorCode::ConfigurationError,
                "Block '" + blockId.toStdString() + "' has no factory",
                "BlockRegistry::createBlock"
            });
        }
        return it.value().factory();
    }

    QVector<BlockDescriptor> blocksByCategory(const QString& category) const {
        QVector<BlockDescriptor> result;
        for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
            if (it.value().category == category) {
                result.push_back(it.value());
            }
        }
        return result;
    }

    QVector<QString> blockIdsByCategory(const QString& category) const {
        QVector<QString> ids;
        for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
            if (it.value().category == category) {
                ids.push_back(it.key());
            }
        }
        return ids;
    }

    QVector<QString> allBlockIds() const {
        QVector<QString> ids;
        for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
            ids.push_back(it.key());
        }
        return ids;
    }

    int blockCount() const { return m_blocks.size(); }

    void clear() {
        m_blocks.clear();
    }

signals:
    void blockRegistered(const QString& blockId, const QString& category);

private:
    BlockRegistry() : QObject(nullptr) {}
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;

    QMap<QString, BlockDescriptor> m_blocks;
};

#define REGISTER_BLOCK(BlockClass, blockId, blockName, blockCategory, blockDescription) \
    static bool BlockClass##_registered = []() { \
        Pipeline::BlockDescriptor desc; \
        desc.id = blockId; \
        desc.name = blockName; \
        desc.category = blockCategory; \
        desc.description = blockDescription; \
        desc.factory = []() -> QObject* { return new BlockClass(); }; \
        Pipeline::BlockRegistry::instance().registerBlock(desc); \
        return true; \
    }();

} // namespace Pipeline

#endif // PIPELINE_BLOCKREGISTRY_H
