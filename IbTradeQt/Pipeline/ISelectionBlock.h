#ifndef PIPELINE_ISELECTIONBLOCK_H
#define PIPELINE_ISELECTIONBLOCK_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

namespace Pipeline {

class ISelectionBlock : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual ~ISelectionBlock() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;

    virtual QJsonObject config() const = 0;
    virtual void setConfig(const QJsonObject& config) = 0;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    virtual QVector<QString> select(const QVector<QString>& universe) = 0;

signals:
    void selectionComplete(const QVector<QString>& candidates);
    void errorOccurred(const QString& message);
};

} // namespace Pipeline

#endif // PIPELINE_ISELECTIONBLOCK_H
