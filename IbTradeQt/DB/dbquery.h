#ifndef DBQUERY_H
#define DBQUERY_H

#include "dbdatatypes.h"
#include <QSqlQuery>

inline QSqlQuery query_addCurrentPosition(const OpenPosition &position) {
    QSqlQuery query;
    query.prepare("INSERT INTO open_positions (strategyId, symbol, quantity, price, pnl, fee, date, status) "
                  "VALUES (:strategyId, :symbol, :quantity, :price, :pnl, :fee, :date, :status)");

    query.bindValue(":strategyId", position.strategyId);
    query.bindValue(":symbol", position.symbol);
    query.bindValue(":quantity", position.quantity);
    query.bindValue(":price", position.price);
    query.bindValue(":pnl", position.pnl);
    query.bindValue(":fee", position.fee);
    query.bindValue(":date", position.date);
    query.bindValue(":status", position.status);

    return query;
}


inline QSqlQuery query_getOpenPositions(const quint16 strategyId) {
    QSqlQuery query;
    query.prepare("SELECT * FROM open_positions WHERE status IN (:status1, :status2) AND strategyId = :strategyId");

    query.bindValue(":status1", PS_INIT_OPEN);
    query.bindValue(":status2", PS_OPEN);
    query.bindValue(":strategyId", strategyId);

    return query;
}

#endif // DBQUERY_H
