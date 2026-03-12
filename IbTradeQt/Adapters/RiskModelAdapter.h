#ifndef ADAPTERS_RISKMODELADAPTER_H
#define ADAPTERS_RISKMODELADAPTER_H

#include <QObject>
#include "Pipeline/IRiskBlock.h"
#include "Strategies/Generic/cbasicriskmodel.h"

class RiskModelAdapter : public Pipeline::IRiskBlock {
    Q_OBJECT

public:
    explicit RiskModelAdapter(CBasicRiskModel* legacyModel,
                              Pipeline::Scope scope = Pipeline::Scope::Strategy,
                              QObject* parent = nullptr)
        : IRiskBlock(parent)
        , m_legacyModel(legacyModel)
        , m_scope(scope)
    {}

    QString id() const override {
        return "legacy-risk-" + m_legacyModel->getId().toString(QUuid::WithoutBraces);
    }
    QString name() const override { return "Legacy Risk: " + m_legacyModel->getName(); }
    Pipeline::Scope scope() const override { return m_scope; }

    QJsonObject config() const override {
        return m_legacyModel->toJson();
    }

    void setConfig(const QJsonObject& config) override {
        m_legacyModel->fromJson(config);
    }

    Pipeline::RiskDecision evaluate(
        const Pipeline::TargetPosition& target,
        const QVector<Pipeline::TargetPosition>& otherTargets) override
    {
        Q_UNUSED(otherTargets)

        DataListPtr data = createDataList();
        UnifiedModelData umd(target.symbol, DIRECTION_UNDEFINED, 0.0,
                             target.targetQuantity, 0.0);
        if (target.targetQuantity > 0)
            umd.direction = DIRECTION_UP;
        else if (target.targetQuantity < 0)
            umd.direction = DIRECTION_DOWN;
        data->append(umd);

        m_legacyModel->processData(data);

        return {Pipeline::RiskDecision::Action::Approve,
                "Legacy risk model approved", {}, id()};
    }

private:
    CBasicRiskModel* m_legacyModel;
    Pipeline::Scope m_scope;
};

#endif // ADAPTERS_RISKMODELADAPTER_H
