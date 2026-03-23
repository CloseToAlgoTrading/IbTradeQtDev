#ifndef ADAPTERS_ALPHAMODELADAPTER_H
#define ADAPTERS_ALPHAMODELADAPTER_H

#include <QObject>
#include "Pipeline/IAlphaBlock.h"
#include "Strategies/Generic/cbasicalphamodel.h"
#include "Strategies/Generic/UnifiedModelData.h"

class AlphaModelAdapter : public Pipeline::IAlphaBlock {
    Q_OBJECT

public:
    explicit AlphaModelAdapter(CBasicAlphaModel* legacyModel, QObject* parent = nullptr)
        : IAlphaBlock(parent)
        , m_legacyModel(legacyModel)
    {
        connect(m_legacyModel, &CBaseModel::dataProcessed,
                this, &AlphaModelAdapter::onLegacyDataProcessed,
                Qt::QueuedConnection);
    }

    QString id() const override {
        return "legacy-alpha-" + m_legacyModel->getId().toString(QUuid::WithoutBraces);
    }
    QString name() const override { return "Legacy Alpha: " + m_legacyModel->getName(); }
    QString description() const override { return "Wrapped CBasicAlphaModel"; }

    QJsonObject config() const override {
        return m_legacyModel->toJson();
    }

    void setConfig(const QJsonObject& config) override {
        m_legacyModel->fromJson(config);
    }

    void initialize() override {
        m_legacyModel->start();
    }

    void shutdown() override {
        m_legacyModel->stop();
    }

    Pipeline::ModelDataList processSemantic(const Pipeline::ModelDataList& in,
                                           const QString& correlationId) override
    {
        m_pendingSemanticCorrelationId = correlationId;
        if (m_legacyModel && in)
            m_legacyModel->processData(in);
        return in;
    }

    bool semanticCompletionIsAsync() const override { return true; }

public slots:
    void onTick(const Pipeline::MarketTick& tick) override {
        m_lastTick = tick;
    }

private slots:
    void onLegacyDataProcessed(DataListPtr data) {
        if (!m_pendingSemanticCorrelationId.isEmpty()) {
            const QString corr = m_pendingSemanticCorrelationId;
            m_pendingSemanticCorrelationId.clear();
            emit semanticReady(data ? data : createDataList(), corr);
            return;
        }

        if (!data || data->isEmpty())
            return;

        for (const auto& umd : *data) {
            Pipeline::Signal signal;
            signal.symbol = umd.symbol;
            signal.confidence = umd.probability;
            signal.direction = convertDirection(umd.direction);
            signal.timestamp = QDateTime::currentDateTime();
            signal.alphaBlockId = id();

            if (signal.direction != Pipeline::Signal::Hold) {
                emit signalGenerated(signal);
            }
        }
    }

private:
    Pipeline::Signal::Direction convertDirection(eDirection legacyDir) {
        switch (legacyDir) {
            case DIRECTION_UP:   return Pipeline::Signal::Buy;
            case DIRECTION_DOWN: return Pipeline::Signal::Sell;
            default:             return Pipeline::Signal::Hold;
        }
    }

    CBasicAlphaModel* m_legacyModel;
    Pipeline::MarketTick m_lastTick;
    QString m_pendingSemanticCorrelationId;
};

#endif // ADAPTERS_ALPHAMODELADAPTER_H
