#ifndef DBDATATYPES_H
#define DBDATATYPES_H

#include <QString>

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
        : id(0), strategyId(0), quantity(0), price(0.0), pnl(0.0), fee(0.0), date(""), status(0) {}
};
#endif // DBDATATYPES_H
