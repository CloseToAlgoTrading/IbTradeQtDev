#ifndef ADAPTERS_EXECUTIONMODELADAPTER_H
#define ADAPTERS_EXECUTIONMODELADAPTER_H

#include <QObject>
#include "Pipeline/IExecutionBlock.h"
#include "Strategies/Generic/cbasicexecutionmodel.h"
#include "Strategies/Generic/UnifiedModelData.h"

class ExecutionModelAdapter : public Pipeline::IExecutionBlock {
    Q_OBJECT

public:
    explicit ExecutionModelAdapter(CBasicExecutionModel* legacyModel, QObject* parent = nullptr)
        : IExecutionBlock(parent)
        , m_legacyModel(legacyModel)
    {}

    QString id() const override {
        return "legacy-exec-" + m_legacyModel->getId().toString(QUuid::WithoutBraces);
    }
    QString name() const override { return "Legacy Execution: " + m_legacyModel->getName(); }

    QJsonObject config() const override {
        return m_legacyModel->toJson();
    }

    void setConfig(const QJsonObject& config) override {
        m_legacyModel->fromJson(config);
    }

public slots:
    void execute(const QVector<Pipeline::ExecutionIntent>& intents) override {
        DataListPtr data = createDataList();

        for (const auto& intent : intents) {
            eDirection dir = (intent.quantity >= 0)
                ? DIRECTION_UP : DIRECTION_DOWN;
            UnifiedModelData umd(
                intent.symbol,
                dir,
                1.0,
                std::abs(intent.quantity),
                0.0
            );
            data->append(umd);
        }

        m_legacyModel->processData(data);

        for (const auto& intent : intents) {
            emit orderPlaced(intent.symbol, intent.correlationId);
        }
    }

private:
    CBasicExecutionModel* m_legacyModel;
};

#endif // ADAPTERS_EXECUTIONMODELADAPTER_H
