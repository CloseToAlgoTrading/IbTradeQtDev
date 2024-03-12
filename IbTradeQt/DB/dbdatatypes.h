#ifndef DBDATATYPES_H
#define DBDATATYPES_H

#include <QString>

enum e_positionStatus {
    PS_INIT_OPEN = 0,
    PS_OPEN,
    PS_CLOSED,
    PS_PARTIALY_CLOSED
};

struct OpenPosition {
    int id;            // Unique identifier, auto-incremented
    int strategyId;    // Identifier for the strategy
    QString symbol; // Trading symbol, up to 10 characters
    int quantity;      // Quantity of the position
    double price;      // Price of the position
    double pnl;        // Profit and Loss
    double fee;        // Associated fee
    QString date;  // Date in text format
    int status;        // Status of the position (open, closed, etc.)

    // Default constructor
    OpenPosition()
        : id(0), strategyId(0), quantity(0), price(0.0), pnl(0.0), fee(0.0), date(""), status(static_cast<int>(PS_INIT_OPEN)) {}
};


struct DbTrade {
    int tradeId;      // Unique identifier for the trade
    QString execId;   // Unique identifier for thetrade execution
    int strategyId;    // Associated Strategy ID
    QString symbol;    // Trading symbol
    int quantity;      // Quantity of the trade
    double price;      // Price at which the trade was executed
    double pnl;        // Profit and Loss for the trade
    double fee;        // Associated fee with the trade
    QString date;      // Date of the trade
    QString tradeType; // Type of the trade (e.g., BUY, SELL)

    // Default constructor
    DbTrade() : tradeId(0), execId(""), strategyId(0), quantity(0), price(0.0), pnl(0.0), fee(0.0), tradeType("BUY") {}
};

#endif // DBDATATYPES_H
