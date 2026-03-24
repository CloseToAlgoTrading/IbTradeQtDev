#ifndef BLOCKS_PASSALLSELECTIONBLOCK_H
#define BLOCKS_PASSALLSELECTIONBLOCK_H

#include <QJsonObject>
#include <QVector>
#include <QString>
#include "../Pipeline/ISelectionBlock.h"

namespace Blocks {

class PassAllSelectionBlock : public Pipeline::ISelectionBlock {
    Q_OBJECT

public:
    explicit PassAllSelectionBlock(QObject* parent = nullptr);

    QString id() const override;
    QString name() const override;
    QString description() const override;

    QJsonObject config() const override;
    void setConfig(const QJsonObject& config) override;
    void initialize() override;
    void shutdown() override;

    QVector<QString> select(const QVector<QString>& universe) override;
};

} // namespace Blocks

#endif // BLOCKS_PASSALLSELECTIONBLOCK_H
