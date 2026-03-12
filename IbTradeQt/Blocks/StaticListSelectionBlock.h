#ifndef BLOCKS_STATICLISTSELECTIONBLOCK_H
#define BLOCKS_STATICLISTSELECTIONBLOCK_H

#include "../Pipeline/ISelectionBlock.h"
#include <QJsonArray>

namespace Blocks {

class StaticListSelectionBlock : public Pipeline::ISelectionBlock {
    Q_OBJECT

public:
    explicit StaticListSelectionBlock(QObject* parent = nullptr)
        : ISelectionBlock(parent) {}

    QString id() const override { return "static-list-selection"; }
    QString name() const override { return "Static List Selection"; }
    QString description() const override { return "Filters universe to a configured list of symbols"; }

    QJsonObject config() const override {
        QJsonObject cfg;
        QJsonArray arr;
        for (const auto& s : m_symbols) arr.append(s);
        cfg["symbols"] = arr;
        return cfg;
    }

    void setConfig(const QJsonObject& config) override {
        m_symbols.clear();
        for (const auto& s : config.value("symbols").toArray())
            m_symbols.append(s.toString());
    }

    void initialize() override {}
    void shutdown() override {}

    QVector<QString> select(const QVector<QString>& universe) override {
        if (m_symbols.isEmpty()) return universe;
        if (universe.isEmpty()) return m_symbols;
        QVector<QString> result;
        for (const auto& s : universe) {
            if (m_symbols.contains(s)) result.append(s);
        }
        return result.isEmpty() ? m_symbols : result;
    }

private:
    QVector<QString> m_symbols;
};

} // namespace Blocks

#endif // BLOCKS_STATICLISTSELECTIONBLOCK_H
