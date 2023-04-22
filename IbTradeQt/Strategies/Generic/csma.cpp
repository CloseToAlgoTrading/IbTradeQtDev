
#include "csma.h"

#include <iostream>

using namespace IBDataTypes;

SMA::SMA(int window) : windowSize(window), sum(0.0) {}

void SMA::calculate(const QList<CHistoricalData>& priceDataList) {
    if (priceDataList.size() < windowSize) {
        std::cout << "Not enough data to calculate SMA." << std::endl;
        return;
    }

    smaList.clear();
    sum = 0.0;

    for (const auto& priceData : priceDataList) {
        sum += priceData.getClose();
        if (priceDataList.indexOf(priceData) >= windowSize) {
            sum -= priceDataList[priceDataList.indexOf(priceData) - windowSize].getClose();
        }
        if (priceDataList.indexOf(priceData) >= windowSize - 1) {
            smaList.append(sum / windowSize);
        }
    }
}

void SMA::update(const PriceData& newPriceData) {
    if (smaList.isEmpty()) {
        std::cout << "Call calculate() method first to initialize the SMA." << std::endl;
        return;
    }

    sum += newPriceData.close - smaList.last() * windowSize;
    smaList.append(sum / windowSize);
}

QVector<double> SMA::getList() const {
    return smaList;
}

int SMA::getWindow() const {
    return windowSize;
}
