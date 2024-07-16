#ifndef DBDATATYPES_H
#define DBDATATYPES_H

#include <QString>
#include <QDateTime>

enum e_positionStatus {
    PS_INIT_OPEN = 0,
    PS_OPEN,
    PS_CLOSED,
    PS_PARTIALY_CLOSED
};

struct OpenPosition {
    int id;            // Unique identifier, auto-incremented
    QString strategyId;// Identifier for the strategy
    QString symbol;    // Trading symbol, up to 10 characters
    int quantity;      // Quantity of the position
    double price;      // Price of the position
    double pnl;        // Profit and Loss
    double fee;        // Associated fee
    QString date;      // Date in text format
    int status;        // Status of the position (open, closed, etc.)

    // Default constructor
    OpenPosition()
        : id(0), strategyId(""), quantity(0), price(0.0), pnl(0.0), fee(0.0), date(""), status(static_cast<int>(PS_INIT_OPEN)) {}
};


struct DbTrade {
    QString execId;   // Unique identifier for thetrade execution
    QString strategyId;    // Associated Strategy ID
    QString symbol;    // Trading symbol
    int quantity;      // Quantity of the trade
    double price;      // Price at which the trade was executed
    double pnl;        // Profit and Loss for the trade
    double fee;        // Associated fee with the trade
    QString date;      // Date of the trade
    QString tradeType; // Type of the trade (e.g., BUY, SELL)

    // Default constructor
    DbTrade() : execId(""), strategyId(""), quantity(0), price(0.0), pnl(0.0), fee(0.0), tradeType("BUY") {}
};

struct DbTradeCommission {
    QString execId;   // Unique identifier for thetrade execution
    double pnl;        // Profit and Loss for the trade
    double fee;        // Associated fee with the trade

    // Default constructor
    DbTradeCommission() : execId(""), pnl(0.0), fee(0.0) {}
};


struct DbStrategyData {
    QString strategyId;    // Strategy ID
    double availableBP;    // Available Buying Power (BP)
    double usedBP;         // Used Buying Power (BP)
    double realizedPnL;    // Realized Profit & Loss (P&L)
    double unrealizedPnL;  // Unrealized Profit & Loss (P&L)
    double pnlPercentage;  // P&L Percentage
    double fees;           // Fees

    // Default constructor
    DbStrategyData()
        : strategyId(""),
        availableBP(0.0),
        usedBP(0.0),
        realizedPnL(0.0),
        unrealizedPnL(0.0),
        pnlPercentage(0.0),
        fees(0.0)
    {}
};

struct DbStrategyInfo {
    QString strategyId;          // Strategy ID
    QString strategyName;        // Strategy Name
    QString strategyDescription; // Strategy Description
    QDateTime createdAt;         // Creation Timestamp
    QDateTime updatedAt;         // Last Updated Timestamp
    QString status;              // Status of the Strategy
    QString currency;            // Currency
    double initialBP;

    // Default constructor
    DbStrategyInfo()
        : strategyId(""),
        strategyName(""),
        strategyDescription(""),
        createdAt(),
        updatedAt(),
        status(""),
        currency(""),
        initialBP(0.0)
    {}
};


#endif // DBDATATYPES_H
