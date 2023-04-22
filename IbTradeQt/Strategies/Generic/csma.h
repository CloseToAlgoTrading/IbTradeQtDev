
#ifndef CSMA_H
#define CSMA_H

#include "GlobalDef.h"
#include <QVector>
#include <QList>
#include "CHistoricalData.h"

using namespace IBDataTypes;
struct PriceData {
    double open;
    double close;
    double high;
    double low;
};

class SMA {
public:
    SMA(int window);

    void calculate(const QList<CHistoricalData>& priceDataList);
    void update(const PriceData& newPriceData);

    QVector<double> getList() const;
    int getWindow() const;

private:
    int windowSize;
    double sum;
    QVector<double> smaList;
};


#endif // CSMA_H
