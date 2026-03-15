#ifndef BACKTEST_FILLEDORDER_H
#define BACKTEST_FILLEDORDER_H

#include <QString>
#include <QDateTime>

namespace Backtest {

struct FilledOrder {
    int         orderId    = 0;
    QString     symbol;
    double      quantity   = 0.0;   // positive = buy, negative = sell
    double      fillPrice  = 0.0;
    double      commission = 0.0;   // zero in v1, field reserved
    double      fees       = 0.0;   // zero in v1, field reserved
    QDateTime   timestamp;
    QString     correlationId;
};

} // namespace Backtest

#endif // BACKTEST_FILLEDORDER_H
