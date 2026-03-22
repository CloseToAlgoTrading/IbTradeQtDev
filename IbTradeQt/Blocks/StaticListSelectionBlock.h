#ifndef BLOCKS_STATICLISTSELECTIONBLOCK_H
#define BLOCKS_STATICLISTSELECTIONBLOCK_H

#include <QJsonObject>
#include <QVector>
#include <QString>
#include "../Pipeline/ISelectionBlock.h"

namespace Blocks {

class StaticListSelectionBlock : public Pipeline::ISelectionBlock {
    Q_OBJECT

public:
    explicit StaticListSelectionBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;

    void initialize() override;
    void shutdown() override;

    QVector<QString> select(const QVector<QString>& universe) override;

private:
    QVector<QString> m_symbols;
};

} // namespace Blocks

#endif // BLOCKS_STATICLISTSELECTIONBLOCK_H
