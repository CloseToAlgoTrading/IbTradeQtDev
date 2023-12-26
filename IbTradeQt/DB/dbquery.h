#ifndef DBQUERY_H
#define DBQUERY_H

#include "dbdatatypes.h"

inline const QString query_addCurrentPosition(const OpenPosition & position)
{
    return QString("INSERT INTO open_positions (strategyId, symbol, quantity, price, pnl, fee, date, status) "
                               "VALUES (%1, '%2', %3, %4, %5, %6, '%7', %8)")
                           .arg(position.strategyId)
                           .arg(position.symbol)
                           .arg(position.quantity)
                           .arg(position.price)
                           .arg(position.pnl)
                           .arg(position.fee)
                           .arg(position.date)
                           .arg(position.status);
}

#endif // DBQUERY_H
