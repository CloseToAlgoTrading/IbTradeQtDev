
#ifndef CBASICALPHAMODEL_H
#define CBASICALPHAMODEL_H

#include "cbasicstrategy_V2.h"
#include "UnifiedModelData.h"
#include <QStringList>



Q_DECLARE_LOGGING_CATEGORY(BasicAlphaModelLog);

class CBasicAlphaModel : public CBasicStrategy_V2
{
    Q_OBJECT
public:
    explicit CBasicAlphaModel(QObject *parent = nullptr);
    virtual ~CBasicAlphaModel() {};

    ModelType modelType() const override { return ModelType::STRATEGY_ALPHA_MODEL; }

public slots:
    virtual void processData(DataListPtr data) override;
    void slotCbkRecvHistoricalData(const QList<IBDataTypes::CHistoricalData> & _histMap, const QString& _symbol);

private:
    // Assuming you have a function to calculate momentum. It could be more complex based on your requirement.
    double calculateMomentum(const QList<CHistoricalData> &data);

    QMap<QString, QList<CHistoricalData>> m_historicalData;
    DataListPtr m_pProcessingData;
    qint16 m_receivedDataCounter;

};

#endif // CBASICALPHAMODEL_H
